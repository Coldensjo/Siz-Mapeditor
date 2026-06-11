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

#ifndef RME_NETWORK_ACTION_H_
#define RME_NETWORK_ACTION_H_

#include "action.h"

class NetworkedActionQueue;

class NetworkedAction : public Action {
public:
	NetworkedAction(Editor& editor, ActionIdentifier ident);
	~NetworkedAction() override;

	uint32_t owner;

protected:
	friend class NetworkedActionQueue;
};

class NetworkedBatchAction : public BatchAction {
	NetworkedActionQueue& queue;

protected:
	NetworkedBatchAction(Editor& editor, NetworkedActionQueue& queue, ActionIdentifier ident);
	~NetworkedBatchAction();

public:
	void addAndCommitAction(Action* action);

protected:
	void commit();
	void undo();
	void redo();

	friend class NetworkedActionQueue;
};

class NetworkedActionQueue : public ActionQueue {
public:
	NetworkedActionQueue(Editor& editor);

	Action* createAction(ActionIdentifier identifier);
	Action* createAction(BatchAction* parent);
	BatchAction* createBatch(ActionIdentifier identifier);

protected:
	void broadcast(DirtyList& dirty_list);

	friend class NetworkedBatchAction;
};

#endif
