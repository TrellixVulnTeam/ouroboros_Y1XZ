// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_MOVETOPOINTHANDLER_H
#define OURO_MOVETOPOINTHANDLER_H

#include "controller.h"
#include "updatable.h"
#include "pyscript/scriptobject.h"	
#include "math/math.h"

namespace Ouroboros{

class MoveToPointHandler : public Updatable
{
public:
	enum MoveType
	{
		MOVE_TYPE_POINT = 0, // regular type
		MOVE_TYPE_ENTITY = 1, // range trigger type
		MOVE_TYPE_NAV = 2, // mobile controller type
	};

	void addToStream(Ouroboros::MemoryStream& s);
	void createFromStream(Ouroboros::MemoryStream& s);

	MoveToPointHandler(OUROShared_ptr<Controller>& pController, int layer, const Position3D& destPos, float velocity, float distance, bool faceMovement, 
		bool moveVertically, PyObject* userarg);

	MoveToPointHandler();
	virtual ~MoveToPointHandler();
	
	virtual bool update();

	virtual const Position3D& destPos() { return destPos_; }
	virtual bool requestMoveOver(const Position3D& oldPos);

	virtual bool isOnGround() { return false; }

	virtual MoveType type() const { return MOVE_TYPE_POINT; }

	void destroy() { isDestroyed_ = true; }

	float velocity() const {
		return velocity_;
	}

	void velocity(float v) {
		velocity_ = v;
	}

protected:
	Position3D destPos_;
	float velocity_; // speed
	bool faceMovement_; // Whether to change the move-oriented
	bool moveVertically_; // true can fly to move or paste
	PyObject* pyuserarg_;
	float distance_;
	OUROShared_ptr<Controller> pController_;
	int layer_;
	bool isDestroyed_;
};
 
}
#endif // OURO_MOVETOPOINTHANDLER_H

