// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#ifndef OURO_UPDATABLES_H
#define OURO_UPDATABLES_H

// common include
#include "helper/debug_helper.h"
#include "common/common.h"
#include "updatable.h"	
// #define NDEBUG
// windows include	
#if OURO_PLATFORM == PLATFORM_WIN32	
#else
// linux include
#endif

namespace Ouroboros{

class Updatables
{
public:
	Updatables();
	~Updatables();

	void clear();

	bool add(Updatable* updatable);
	bool remove(Updatable* updatable);

	void update();

private:
	std::vector< std::map<uint32, Updatable*> > objects_;
};

}
#endif
