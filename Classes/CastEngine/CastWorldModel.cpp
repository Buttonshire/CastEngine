//
//  CastWorldModel.cpp
//  CastEngine
//
//  Created by Paul Zirkle on 6/5/13.
//
//

#include "CastWorldModel.h"

#include "CastCommandTime.h"


//static
CastWorldModel* CastWorldModel::s_instance = NULL;

//static
CastWorldModel* CastWorldModel::get()
{
	if( s_instance == NULL )
	{
		s_instance = new CastWorldModel();
	}
	
	return s_instance;
}

CastWorldModel::CastWorldModel()
{
	
}

CastWorldModel::~CastWorldModel()
{
	
}

void CastWorldModel::addEffectInTransit( ICastEntity* from, CastEffect* effect, CastTarget* targetList, double startTime, double speed   )
{
	
	if( speed == 0.0 )
	{
		addEffectInstant(from, effect, targetList, startTime);
		return;
	}
	
	if( targetList->getType() == CTT_ENTITIES )
	{
		const std::vector<ICastEntity*>& targets = targetList->getEntityList();
		for( int i=0; i< targets.size(); i++)
		{
			ICastEntity* target = targets[i];
			
			CCLOG("add effect in transit" );
			
			CastEffectPath path;
			path.from = from;
			path.to = target;
			path.speed = speed;
			path.startTime = startTime;
			path.effect = effect;
			effect->retain();
			m_effectsInTransit.push_back(path);
		}
	}else {
		//TODO: world position
	}
	
	//CC_SAFE_RELEASE(effect); 
	

	//TODO: scheduler?
}

void CastWorldModel::addEffectInstant(  ICastEntity* from, CastEffect* effect, CastTarget* targetList, double startTime  )
{
	
	if( targetList->getType() == CTT_ENTITIES )
	{
		const std::vector<ICastEntity*>& targets = targetList->getEntityList();
		for( int i=0; i< targets.size(); i++)
		{
			ICastEntity* target = targets[i];
			
			CastEffectPath path;
			path.from = from;
			path.to = target;
			path.speed = 0.0f;
			path.startTime = startTime;
			path.effect = effect;
			effect->retain();

			applyEffectToTarget( path );
			
		}
	}else {
		//TODO: world position
	}


	
	//CC_SAFE_RELEASE(effect); 
}

void CastWorldModel::applyEffectToTarget( CastEffectPath path )
{
	CCLOG("apply effect to target");
	double currTime = CastCommandTime::get();

	CastEffect* effect = path.effect;
	std::vector<ICastEntity*> targets;
	if( path.to != NULL )  {
		targets.push_back(path.to);
	}else {
		//if targeted position, check physics to determine targets
		CCLog("todo: physics check at position for effect targets");
	}

	for( int t=0; t< targets.size(); t++)
	{
		CastEffect* eff = effect;
		if( t > 0 ) eff = effect->clone();  //targets might modify the effect, so give each target it's own copy

		eff->setTarget( targets[t] );
		eff->m_startTime = currTime; //start the clock on effect's life time

		targets[t]->applyEffect( eff );
	}

}

void CastWorldModel::updateStep( float dt )
{
	std::vector<int> resolvedPaths;

	double currTime = CastCommandTime::get();
	for( int i=0; i< m_effectsInTransit.size(); i++) 
	{
		//TODO: if blockable, check physics collision
		CastEffectPath path = m_effectsInTransit[i];

		float distToTargetFromOrigin = 1.0f; //TODO: add physics checks
		float timeToTargetFromOrigin = distToTargetFromOrigin / path.speed;

		if( currTime - path.startTime >= timeToTargetFromOrigin )
		{
			applyEffectToTarget(path);
			
			//effect path reached target
			resolvedPaths.push_back(i);
		}
	}

	//clean up resolved paths
	for( int i= resolvedPaths.size()-1; i>=0; i--)
	{
		m_effectsInTransit[i].effect->release();
		m_effectsInTransit.erase( m_effectsInTransit.begin() + i );
	}
}

