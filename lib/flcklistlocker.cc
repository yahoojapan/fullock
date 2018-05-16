/*
 * FULLOCK - Fast User Level LOCK library
 *
 * Copyright 2015 Yahoo Japan Corporation.
 *
 * FULLOCK is fast locking library on user level by Yahoo! JAPAN.
 * FULLOCK is following specifications.
 *
 * For the full copyright and license information, please view
 * the license file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Mon 6 Jul 2015
 * REVISION:
 *
 */

#include "flcklistlocker.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// FlListLocker class
//---------------------------------------------------------
void FlListLocker::dump(std::ostream &out, int level) const
{
	if(!pcurrent){
		return;
	}
	string	spacer1 = spaces_string(level);
	string	spacer2 = spaces_string(level + 1);

	out << spacer1 << "FLLOCKER(" << to_hexstring(pcurrent) << ")={" << std::endl;

	fllistbaselocker::dump(out, level + 1);

	out << spacer2 << "flckpid           = " << pcurrent->flckpid	<< std::endl;
	out << spacer2 << "fd                = " << pcurrent->fd		<< std::endl;
	out << spacer2 << "locked            = " << (pcurrent->locked ? "locked" : "not locked")	<< std::endl;
	out << spacer1 << "}" << std::endl;
}

bool FlListLocker::check_dead_lock(dev_t devid, ino_t inoid, fl_pid_cache_map_t* pcache, flckpid_t except_flckpid, int except_fd)
{
	if(!pcurrent){
		ERR_FLCKPRN("Object is not initialized or parameters are wrong.");
		return false;
	}
	if(pcurrent->flckpid == except_flckpid && pcurrent->fd == except_fd){
		return false;
	}
	dev_t	tgdev = FLCK_INVALID_ID;
	ino_t	tgino = FLCK_INVALID_ID;
	if(!GetFileDevNode(pcurrent->flckpid, pcurrent->fd, tgdev, tgino, pcache)){
		// not found this
		return true;
	}else{
		if((static_cast<dev_t>(FLCK_EACCESS_ID) != tgdev && static_cast<ino_t>(FLCK_EACCESS_ID) != tgino) && (tgdev != devid || tgino != inoid)){
			// [NOTE]
			// We do not check devid and inodeid, when could not access to pid/tid/fd in /proc.
			// Then devid and inodeid is returned FLCK_EACCESS_ID.
			//
			// [NOTE: For NO FD]
			// On no fd mode both devid and inoid are FLCK_EACCESS_ID. If thread(pid/tid) is exited, GetFileDevNode returns
			// FLCK_INVALID_ID as tgdev and tgino.
			// Then come here and find dead locking.
			//
			return true;
		}
	}
	return false;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
