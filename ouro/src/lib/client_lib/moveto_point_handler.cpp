// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#include "entity.h"
#include "moveto_point_handler.h"	

namespace Ouroboros{	
namespace client
{

//-------------------------------------------------------------------------------------
MoveToPointHandler::MoveToPointHandler(ScriptCallbacks& scriptCallbacks, client::Entity* pEntity, 
											int layer, const Position3D& destPos, 
											 float velocity, float distance, bool faceMovement, 
											bool moveVertically, PyObject* userarg):
ScriptCallbackHandler(scriptCallbacks, NULL),
destPos_(destPos),
velocity_(velocity),
faceMovement_(faceMovement),
moveVertically_(moveVertically),
pyuserarg_(userarg),
distance_(distance),
layer_(layer),
pEntity_(pEntity)
{
}

//-------------------------------------------------------------------------------------
MoveToPointHandler::~MoveToPointHandler()
{
	if(pyuserarg_ != NULL)
	{
		Py_DECREF(pyuserarg_);
	}

	// DEBUG_MSG(fmt::format("MoveToPointHandler::~MoveToPointHandler(): {:p}\n"), (void*)this));
}

//-------------------------------------------------------------------------------------
void MoveToPointHandler::handleTimeout( TimerHandle handle, void * pUser )
{
	update(handle);
}

//-------------------------------------------------------------------------------------
void MoveToPointHandler::onRelease( TimerHandle handle, void * /*pUser*/ )
{
	scriptCallbacks_.releaseCallback(handle);
	delete this;
}

//-------------------------------------------------------------------------------------
bool MoveToPointHandler::requestMoveOver(TimerHandle& handle, const Position3D& oldPos)
{
	pEntity_->onMoveOver(scriptCallbacks_.getIDForHandle(handle), layer_, oldPos, pyuserarg_);
	handle.cancel();
	return true;
}

//-------------------------------------------------------------------------------------
bool MoveToPointHandler::update(TimerHandle& handle)
{
	if(pEntity_ == NULL)
	{
		handle.cancel();
		return false;
	}
	
	Entity* pEntity = pEntity_;
	const Position3D& dstPos = destPos();
	Position3D currpos = pEntity->position();
	Position3D currpos_backup = currpos;
	Direction3D direction = pEntity->direction();

	Vector3 movement = dstPos - currpos;
	if (!moveVertically_) movement.y = 0.f;
	
	bool ret = true;

	if(OUROVec3Length(&movement) < velocity_ + distance_)
	{
		float y = currpos.y;
		currpos = dstPos;

		if(distance_ > 0.0f)
		{
			// unitized vector
			OUROVec3Normalize(&movement, &movement); 
			movement *= distance_;
			currpos -= movement;
		}

		if (!moveVertically_)
			currpos.y = y;

		ret = false;
	}
	else
	{
		// unitized vector
		OUROVec3Normalize(&movement, &movement); 

				// move Place
		movement *= velocity_;
		currpos += movement;
	}
	
	// Do you need to change your orientation?
	if (faceMovement_ && (movement.x != 0.f || movement.z != 0.f))
		direction.yaw(movement.yaw());
	
	// Set the new location and orientation of the entity
	pEntity_->clientPos(currpos);
	pEntity_->clientDir(direction);

	// non-navigate can't be sure it's on the ground
	pEntity_->isOnGround(false);

	// notification script
	pEntity->onMove(scriptCallbacks_.getIDForHandle(handle), layer_, currpos_backup, pyuserarg_);

	// return true if the destination is reached
	if(!ret)
	{
		return !requestMoveOver(handle, currpos_backup);
	}

	return true;
}

//-------------------------------------------------------------------------------------
}
}
