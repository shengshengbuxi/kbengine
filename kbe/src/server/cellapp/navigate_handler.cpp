// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com

#include "cellapp.h"
#include "entity.h"
#include "navigate_handler.h"	
#include "navigation/navigation.h"

namespace KBEngine{	


//-------------------------------------------------------------------------------------
NavigateHandler::NavigateHandler(KBEShared_ptr<Controller>& pController, const Position3D& destPos, 
											 float velocity, float distance, bool faceMovement, 
											 float maxMoveDistance, VECTOR_POS3D_PTR paths_ptr,
											PyObject* userarg):
MoveToPointHandler(pController, pController->pEntity()->layer(), pController->pEntity()->position(), velocity, distance, faceMovement, false, userarg),
destPosIdx_(0),
paths_(paths_ptr),
maxMoveDistance_(maxMoveDistance)
{
	destPos_ = (*paths_)[destPosIdx_++];
	
	updatableName = "NavigateHandler";
}

//-------------------------------------------------------------------------------------
NavigateHandler::NavigateHandler():
MoveToPointHandler(),
destPosIdx_(0),
paths_(),
maxMoveDistance_(0.f)
{
	updatableName = "NavigateHandler";
}

//-------------------------------------------------------------------------------------
NavigateHandler::~NavigateHandler()
{
}

//-------------------------------------------------------------------------------------
void NavigateHandler::addToStream(KBEngine::MemoryStream& s)
{
	MoveToPointHandler::addToStream(s);
	s << maxMoveDistance_;
}

//-------------------------------------------------------------------------------------
void NavigateHandler::createFromStream(KBEngine::MemoryStream& s)
{
	MoveToPointHandler::createFromStream(s);
	s >> maxMoveDistance_;
}

//-------------------------------------------------------------------------------------
bool NavigateHandler::requestMoveOver(const Position3D& oldPos)
{
	if(destPosIdx_ == ((int)paths_->size()))
		return MoveToPointHandler::requestMoveOver(oldPos);
	else
		destPos_ = (*paths_)[destPosIdx_++];

	return false;
}

//-------------------------------------------------------------------------------------
bool NavigateHandler::update()
{
	if (isDestroyed_)
	{
		delete this;
		return false;
	}

	float tick = 1.f;

	int numOfLoops = 0;
	float distance = distance_;

	bool ret = true;

	do
	{

		distance_ = 0;

		if (destPosIdx_ >= ((int)paths_->size()))
			distance_ = distance;

		ret = move(&tick);

		if (ret) 
		{
			distance_ = distance;

			if (++numOfLoops > 100)
			{

				if(pController_ && pController_->pEntity())
					pController_->pEntity()->onMoveFailure(pController_->id(), pyuserarg_);

			
				if(pController_)
					pController_->destroy();
		
				pController_.reset();
			}
		}
		
	}	while (!almostZero(tick) && ret);


	//if (isDestroyed_)
	//{
	//	delete this;
	//	return false;
	//}
	//

	//float distance = distance_;
	//float velocity = velocity_;

	//bool ret = true;
	//float remainingTicks = 1.f;

	//while (ret && remainingTicks  > 0.f)
	//{		
	//	if (isDestroyed_)
	//	{
	//		delete this;
	//		return false;
	//	}

	//	
	//	Entity* pEntity = pController_->pEntity();
	//	Position3D pos = pEntity->position();



	//	int oldDestPosIdx = destPosIdx_;


	//	if (destPosIdx_ < (int)paths_->size()-1) 
	//	{
	//		distance_ = 0;
	//	}
	//	else
	//	{
	//		distance_ = distance;
	//	}

	//	
	//	velocity_ = remainingTicks * velocity;


	//	ret = MoveToPointHandler::update();

	//	if (ret)
	//	{
	//		distance_ = distance;
	//		
	//		Vector3 movement = pEntity->position() - pos;
	//			
	//		if (!moveVertically_) movement.y = 0.f;
	//			
	//		float dist_len = KBEVec3Length(&movement);

	//
	//		remainingTicks -= dist_len / velocity;

	//		velocity_ = velocity;
	//	}

	//};


	//
	return ret;
}

//-------------------------------------------------------------------------------------
}

