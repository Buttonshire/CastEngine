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

void CastWorldModel::addEffectInTransit( ICastEntity* from, CastEffect* effect, CastTarget* targetList, double startTime  )
{
	float speed = effect->getTravelSpeed();
	if( speed == 0.0 )
	{
		addEffectInstant(from, effect, targetList, startTime);
		return;
	}
	
	if( targetList->getType() == CTT_ENTITIES )
	{
		CastEffectPath path;
		path.from = from;
			
		path.speed = speed;
		path.startTime = startTime;

		const std::vector<ICastEntity*>& targets = targetList->getEntityList();
		for( int i=0; i< targets.size(); i++)
		{
			ICastEntity* target = targets[i];
			
			if( !CastWorldModel::get()->isValid( from ) ) continue;
			if( !CastWorldModel::get()->isValid( target ) ) continue;
			CCLOG("add effect in transit" );
			
			path.to = target;
			path.effects.push_back(effect);
			effect->retain();
			
		}

		m_effectsInTransit.push_back(path);

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
		CastEffectPath path;
		path.from = from;
		path.speed = 0.0f;
		path.startTime = startTime;

		const std::vector<ICastEntity*>& targets = targetList->getEntityList();
		for( int i=0; i< targets.size(); i++)
		{
			ICastEntity* target = targets[i];

			if( !CastWorldModel::get()->isValid( from ) ) continue;
			if( !CastWorldModel::get()->isValid( target ) ) continue;
			

			path.to = target;

			path.effects.push_back(effect);
			effect->retain();

			
			
		}

		applyEffectToTarget( path );

	}else {
		//TODO: world position
	}


	
	//CC_SAFE_RELEASE(effect); 
}

void CastWorldModel::applyEffectToTarget( CastEffectPath path )
{
	CCLOG("apply effects to target");
	double currTime = CastCommandTime::get();

	for( int i=0; i< path.effects.size(); i++) 
	{
		CastEffect* effect = path.effects[i];

		std::vector<ICastEntity*> targets;
		if( path.to != NULL )  {
			if( CastWorldModel::get()->isValid( path.to ) ) {
				targets.push_back(path.to);
			}
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
		for( int e= m_effectsInTransit[i].effects.size() -1; e>=0; e-- ) {
			m_effectsInTransit[i].effects[e]->release();
		}
		m_effectsInTransit[i].effects.clear();


		m_effectsInTransit.erase( m_effectsInTransit.begin() + i );
	}
}

bool CastWorldModel::isValid( ICastEntity* entity )
{
	return (m_allEntities.count(entity) > 0 );
}