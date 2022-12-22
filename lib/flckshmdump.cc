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
 * CREATE:   Mon 15 Jun 2015
 * REVISION:
 *
 */

#include <string>
#include <iostream>

#include "flckcommon.h"
#include "flckshm.h"
#include "flckstructure.h"
#include "flcklistlocker.h"
#include "flcklistofflock.h"
#include "flcklistfilelock.h"
#include "flcklistnmtx.h"
#include "flcklistncond.h"
#include "flcklistwaiter.h"
#include "flckutil.h"
#include "flckdbg.h"

using namespace std;
using namespace fullock;

//---------------------------------------------------------
// FlShm : Dump Methods
//---------------------------------------------------------
bool FlShm::Dump(ostream& out, bool is_free_list)
{
	out << "[SHM]============================================" << std::endl;

	if(FLCK_INVALID_HANDLE == FlShm::ShmFd){
		out << "[SHM] pFlHead                   = Not initialized" << std::endl;
		return true;
	}

	// dump: FLHEAD
	out << "[SHM] version                   = "	<< to_hexstring(FlShm::pFlHead->version)			<< std::endl;
	out << "[SHM] szver                     = "	<< FlShm::pFlHead->szver							<< std::endl;
	out << "[SHM] flength                   = "	<< FlShm::pFlHead->flength							<< std::endl;
	out << "[SHM] file_lock_list            = "	<< to_hexstring(FlShm::pFlHead->file_lock_list)		<< std::endl;
	out << "[SHM] named_mutex_list          = "	<< to_hexstring(FlShm::pFlHead->named_mutex_list)	<< std::endl;
	out << "[SHM] file_lock_free            = "	<< to_hexstring(FlShm::pFlHead->file_lock_free)		<< std::endl;
	out << "[SHM] offset_lock_free          = "	<< to_hexstring(FlShm::pFlHead->offset_lock_free)	<< std::endl;
	out << "[SHM] locker_free               = "	<< to_hexstring(FlShm::pFlHead->locker_free)		<< std::endl;
	out << "[SHM] named_mutex_free          = "	<< to_hexstring(FlShm::pFlHead->named_mutex_free)	<< std::endl;

	// dump: file_lock_list
	out << "[file_lock_list]={" << std::endl;
	if(FlShm::pFlHead->file_lock_list){
		FlListFileLock	list(to_abs(FlShm::pFlHead->file_lock_list));
		list.dump(out, 1);
	}
	out << "}" << std::endl;

	// dump: named_mutex_list
	out << "[named_mutex_list]={" << std::endl;
	if(FlShm::pFlHead->named_mutex_list){
		FlListNMtx	list(to_abs(FlShm::pFlHead->named_mutex_list));
		list.dump(out, 1);
	}
	out << "}" << std::endl;

	// dump: named_cond_list
	out << "[named_cond_list]={" << std::endl;
	if(FlShm::pFlHead->named_cond_list){
		FlListNCond	list(to_abs(FlShm::pFlHead->named_cond_list));
		list.dump(out, 1);
	}
	out << "}" << std::endl;

	if(is_free_list){
		// dump: file_lock_free
		out << "[file_lock_free]={" << std::endl;
		for(PFLFILELOCK ptmp = to_abs(FlShm::pFlHead->file_lock_free); ptmp; ptmp = to_abs(ptmp->next)){
			FlListFileLock	list(ptmp);
			list.dump(out, 1);
		}
		out << "}" << std::endl;

		// dump: offset_lock_free
		out << "[offset_lock_free]={" << std::endl;
		for(PFLOFFLOCK ptmp = to_abs(FlShm::pFlHead->offset_lock_free); ptmp; ptmp = to_abs(ptmp->next)){
			FlListOffLock	list(ptmp);
			list.dump(out, 1);
		}
		out << "}" << std::endl;

		// dump: locker_free
		out << "[locker_free]={" << std::endl;
		for(PFLLOCKER ptmp = to_abs(FlShm::pFlHead->locker_free); ptmp; ptmp = to_abs(ptmp->next)){
			FlListLocker	list(ptmp);
			list.dump(out, 1);
		}
		out << "}" << std::endl;

		// dump: named_mutex_free
		out << "[named_mutex_free]={" << std::endl;
		for(PFLNAMEDMUTEX ptmp = to_abs(FlShm::pFlHead->named_mutex_free); ptmp; ptmp = to_abs(ptmp->next)){
			FlListNMtx	list(ptmp);
			list.dump(out, 1);
		}
		out << "}" << std::endl;

		// dump: named_mutex_free
		out << "[named_cond_free]={" << std::endl;
		for(PFLNAMEDCOND ptmp = to_abs(FlShm::pFlHead->named_cond_free); ptmp; ptmp = to_abs(ptmp->next)){
			FlListNCond	list(ptmp);
			list.dump(out, 1);
		}
		out << "}" << std::endl;

		// dump: waiter_free
		out << "[waiter_free]={" << std::endl;
		for(PFLWAITER ptmp = to_abs(FlShm::pFlHead->waiter_free); ptmp; ptmp = to_abs(ptmp->next)){
			FlListWaiter	list(ptmp);
			list.dump(out, 1);
		}
		out << "}" << std::endl;
	}
	return true;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
