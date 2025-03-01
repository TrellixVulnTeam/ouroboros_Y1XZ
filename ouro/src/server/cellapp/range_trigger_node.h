// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_RANGE_TRIGGER_NODE_H
#define OURO_RANGE_TRIGGER_NODE_H

#include "range_trigger.h"
#include "coordinate_node.h"
#include "math/math.h"

namespace Ouroboros{


class RangeTriggerNode : public CoordinateNode
{
public:
	RangeTriggerNode(RangeTrigger* pRangeTrigger, float xz, float y, bool positiveBoundary);
	virtual ~RangeTriggerNode();

	INLINE void range(float xz, float y);
	INLINE void old_range(float xz, float y);
	INLINE float range_xz() const;
	INLINE float range_y() const;

	INLINE RangeTrigger* pRangeTrigger() const;
	INLINE void pRangeTrigger(RangeTrigger* pRangeTrigger);

	/**
		(extended coordinates)
		x && z is implemented by different applications (from different locations)
	*/
	virtual float xx() const;
	virtual float yy() const;
	virtual float zz() const;

	INLINE bool isInXRange(CoordinateNode * pNode);
	INLINE bool isInYRange(CoordinateNode * pNode);
	INLINE bool isInZRange(CoordinateNode * pNode);

	INLINE bool wasInXRange(CoordinateNode * pNode);
	bool wasInYRange(CoordinateNode * pNode);
	INLINE bool wasInZRange(CoordinateNode * pNode);

	virtual void resetOld()
	{ 
		CoordinateNode::resetOld();
		old_range_xz_ = range_xz_;
		old_range_y_ = range_y_;
	}

	/**
		Parent node deletion
	*/
	virtual void onParentRemove(CoordinateNode* pParentNode);

	virtual void onRemove();

	void onTriggerUninstall();

	/**
		A node has passed this node
		@isfront: Move forward or backward
	*/
	virtual void onNodePassX(CoordinateNode* pNode, bool isfront);
	virtual void onNodePassY(CoordinateNode* pNode, bool isfront);
	virtual void onNodePassZ(CoordinateNode* pNode, bool isfront);

	INLINE bool isPositive() const;

protected:
	float range_xz_, range_y_, old_range_xz_, old_range_y_;
	RangeTrigger* pRangeTrigger_;
};
}

#ifdef CODE_INLINE
#include "range_trigger_node.inl"
#endif
#endif
