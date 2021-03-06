/*
 * Particle.cpp
 *
 *  Created on: Jul 6, 2014
 *      Author: user
 */

#include "Particle.h"
#include "Map.h"
#include "Robot.h"
#include "Point.h"
#include "config.h"

Particle::Particle(Point p_pPoint,double p_dYaw,double p_dBel, Map* p_mMap)
			:_particleMap(p_mMap),
			 _particleLocation(p_pPoint),
			 _particleYaw(p_dYaw),
			 _particleBelief(p_dBel)
{
}

void Particle::update(Point p_dDeltaLocation, double p_dDeltaYaw, Robot* robot)
{
	// Update The location according to delta
	_particleLocation.x += p_dDeltaLocation.x;
	_particleLocation.y += p_dDeltaLocation.y;
	_particleYaw += p_dDeltaYaw;

	// Robot path posterior
	double probOfPath = this->getParticleProbability(p_dDeltaLocation, p_dDeltaYaw);

	// Robot observations posterior
	double ProbOfObs = this->getParticleObservationsProbablity(robot);

	// Calculate new belief according to path, observations and previous belief
	_particleBelief = SAFETY_FACTORY * _particleBelief * probOfPath * ProbOfObs;

	// Check to see if particle exceeded the maximum probability
	if (_particleBelief > 1)
	{
		_particleBelief = 1;
	}
}

double Particle::getParticleProbability(Point p_pLocation, double p_dDeltaYaw)
{
	double distance = sqrt(pow(p_pLocation.x, 2) + pow(p_pLocation.y, 2));

	// Kept moving forward
	if ((p_dDeltaYaw == 0) && (distance < DISTANCE_LIMIT))
	{
		return 1;
	}
	else if ((distance < DISTANCE_LIMIT) && (p_dDeltaYaw < YAW_LIMIT))
	{
		return 0.8;
	}
	else if (((distance > DISTANCE_LIMIT) && (p_dDeltaYaw < YAW_LIMIT)) ||
			 ((distance < DISTANCE_LIMIT) && (p_dDeltaYaw > YAW_LIMIT)))
	{
		return 0.6;
	}
	else
	{
		return 0.2;
	}
}

double Particle::getParticleObservationsProbablity(Robot* robot)
{
	double trueNum = 0;
	double falseNum = 0;

	// set the current location as free
	_particleMap->setValueAtLocation(_particleLocation, STATE_FREE);

	Point pObstacleLocation;

	for (int currSensor = 0; currSensor < LASER_SENSORS_NUM; currSensor+=10)
	{
		double laserAngle = GET_LASER_ANGLE(currSensor);

		// Calculating distance from obstacle
		double distance = robot->getDistanceFromObstacle(currSensor);
		pObstacleLocation.x = distance * cos(_particleYaw + laserAngle) + _particleLocation.x;
		pObstacleLocation.y = distance * sin(_particleYaw + laserAngle) + _particleLocation.y;

		// Calculate distance in both axes
		double progressAngle = atan((pObstacleLocation.y - _particleLocation.y) / (pObstacleLocation.x - _particleLocation.x));
		double xAdd = abs(cos(progressAngle) / 3.0);

		if (pObstacleLocation.x < _particleLocation.x)
		{
			xAdd = -xAdd;
		}

		double yAdd = abs(sin(progressAngle) / 3.0);

		if (pObstacleLocation.y < _particleLocation.y)
		{
			yAdd = -yAdd;
		}

		Point p;
		for (double j=0; j < distance * 3; ++j)
		{
			p.x = _particleLocation.x + j*xAdd;
			p.y = _particleLocation.y + j*yAdd;
			_particleMap->setValueAtLocation(p, STATE_FREE);
		}

		MatrixCell currCell = _particleMap->getValueAtLocation(pObstacleLocation);

		if (distance < 4.0)
		{
			if (currCell.GetStatus() == STATE_FREE)
			{
				falseNum++;
				_particleMap->setValueAtLocation(pObstacleLocation, STATE_OCCUPIED);
			}
			else if (currCell.GetStatus() == STATE_OCCUPIED)
			{
				trueNum++;
			}
		}
	}

	if (falseNum == 0)
	{
		return 1;
	}
	else
	{
		return trueNum / (trueNum + falseNum);
	}
}

Particle* Particle::CreateParticle()
{
	double newX = RANDOMIZE_LOCATION(_particleLocation.x);
	double newY = RANDOMIZE_LOCATION(_particleLocation.y);
	double newYaw = RANDOMIZE_YAW(_particleYaw);
	return new Particle(Point(newX, newY), newYaw, _particleBelief, new Map(*_particleMap));
}

Map* Particle::getMap()
{
	return this->_particleMap;
}

double Particle::getBelief()
{
	return this->_particleBelief;
}

Particle::~Particle()
{
	if (_particleMap != NULL)
		delete _particleMap;
}
