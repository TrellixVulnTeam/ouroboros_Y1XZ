// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_CLIENT_MOVETOPOINTHANDLER_H
#define OURO_CLIENT_MOVETOPOINTHANDLER_H

#include "pyscript/scriptobject.h"	
#include "math/math.h"
#include "script_callbacks.h"

namespace Ouroboros{
namespace client
{

class Entity;
class MoveToPointHandler : public ScriptCallbackHandler
{
public:
	enum MoveType
	{
		MOVE_TYPE_POINT = 0, // regular type
		MOVE_TYPE_ENTITY = 1, // range trigger type
		MOVE_TYPE_NAV = 2, // mobile controller type
	};

	MoveToPointHandler(ScriptCallbacks& scriptCallbacks, client::Entity* pEntity, int layer, 
		const Position3D& destPos, float velocity, float distance, bool faceMovement, 
		bool moveVertically, PyObject* userarg);

	MoveToPointHandler();
	virtual ~MoveToPointHandler();
	
	virtual bool update(TimerHandle& handle);

	virtual const Position3D& destPos(){ return destPos_; }
	virtual bool requestMoveOver(TimerHandle& handle, const Position3D& oldPos);

	virtual bool isOnGround(){ return false; }

	virtual MoveType type() const{ return MOVE_TYPE_POINT; }

protected:
	virtual void handleTimeout( TimerHandle handle, void * pUser );
	virtual void onRelease( TimerHandle handle, void * /*pUser*/ );

protected:
	Position3D destPos_;
	float velocity_; // speed
	bool faceMovement_; // Whether to change the move-oriented
	bool moveVertically_; // true can fly to move or paste
	PyObject* pyuserarg_;
	float distance_;
	int layer_;
	client::Entity* pEntity_;
};
 
}
}
#endif // OURO_CLIENT_MOVETOPOINTHANDLER_H

