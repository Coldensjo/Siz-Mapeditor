//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_action.h"
#include "editor.h"

NetworkedAction::NetworkedAction(Editor& editor, ActionIdentifier ident) :
	Action(editor, ident),
	owner(0) {
	;
}

NetworkedAction::~NetworkedAction() {
	;
}

NetworkedBatchAction::NetworkedBatchAction(Editor& editor, NetworkedActionQueue& queue, ActionIdentifier ident) :
	BatchAction(editor, ident),
	queue(queue) {
	;
}

NetworkedBatchAction::~NetworkedBatchAction() {
	;
}

void NetworkedBatchAction::addAndCommitAction(Action* action) {
	// If empty, do nothing.
	if (action->size() == 0) {
		delete action;
		return;
	}

	// Track changed nodes...
	DirtyList dirty_list;
	NetworkedAction* netact = dynamic_cast<NetworkedAction*>(action);
	if (netact) {
		dirty_list.owner = netact->owner;
	}

	const bool trackChanges = type != ACTION_SELECT && type != ACTION_REMOTE;
	action->commit(trackChanges ? &dirty_list : nullptr);
	batch.push_back(action);
	timestamp = time(nullptr);

	// Broadcast local edits only; remote/streamed changes must not echo back.
	if (trackChanges && !dirty_list.Empty()) {
		queue.broadcast(dirty_list);
	}
}

void NetworkedBatchAction::commit() {
	// Track changed nodes...
	DirtyList dirty_list;
	const bool trackChanges = type != ACTION_SELECT && type != ACTION_REMOTE;

	for (ActionVector::iterator it = batch.begin(); it != batch.end(); ++it) {
		NetworkedAction* action = static_cast<NetworkedAction*>(*it);
		if (!action->isCommited()) {
			action->commit(trackChanges ? &dirty_list : nullptr);
			if (action->owner != 0) {
				dirty_list.owner = action->owner;
			}
		}
	}
	if (trackChanges && !dirty_list.Empty()) {
		queue.broadcast(dirty_list);
	}
}

void NetworkedBatchAction::undo() {
	// Track changed nodes...
	DirtyList dirty_list;
	const bool trackChanges = type != ACTION_SELECT && type != ACTION_REMOTE;

	for (ActionVector::reverse_iterator it = batch.rbegin(); it != batch.rend(); ++it) {
		(*it)->undo(trackChanges ? &dirty_list : nullptr);
	}
	if (trackChanges && !dirty_list.Empty()) {
		queue.broadcast(dirty_list);
	}
}

void NetworkedBatchAction::redo() {
	commit();
}

//===================
// Action queue

NetworkedActionQueue::NetworkedActionQueue(Editor& editor) :
	ActionQueue(editor) {
}

Action* NetworkedActionQueue::createAction(ActionIdentifier identifier) {
	return newd NetworkedAction(editor, identifier);
}

Action* NetworkedActionQueue::createAction(BatchAction* parent) {
	return newd NetworkedAction(editor, parent->getType());
}

BatchAction* NetworkedActionQueue::createBatch(ActionIdentifier identifier) {
	return newd NetworkedBatchAction(editor, *this, identifier);
}

void NetworkedActionQueue::broadcast(DirtyList& dirty_list) {
	editor.BroadcastNodes(dirty_list);
}
