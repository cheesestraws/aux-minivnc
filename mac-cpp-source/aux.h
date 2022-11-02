#if !defined(__mac_aux_h)
#define __mac_aux_h

/*
 * Copyright 1987-91 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
 * The copyright notice above does not evidence any actual or
 * intended publication of such source code.
 */

/*
    C Interface to the Macintosh Libraries
*/

#include <Events.h>

/*
    These constants define selectors for the AUXDispatch call.  This call
should be used only after using SysEnviron to determine that the application
is running under A/UX.
*/

#define AUX_HIGHEST	0	/* return highest available selector */
				/* p is not used */
#define AUX_GET_ERRNO	1	/* get pointer to errno */
				/* p points to (int *) */
#define AUX_GET_PRINTF	2	/* get pointer to printf() */
				/* p points to ((*int) ()) */
#define AUX_GET_SIGNAL	3	/* get pointer to signal() */
				/* p points to ((*int) ()) */
#define AUX_GET_TIMEOUT	4	/* return nice timeout value in clock ticks */
				/* p is not used */
#define AUX_SET_SELRECT	5	/* set a mouse movement rectangle to use in */
				/* future select() calls.  p points to a Rect */
#define AUX_CHECK_KIDS	6	/* check to see if a given process has kids */
				/* p points to an integer containing the */
				/* process id */
#define AUX_POST_MODIFIED 7	/* post an event, with modifiers */

#define AUX_FIND_EVENT	8	/* search event queue for an event */

#define AUX_GET_UDEVFD  9       /* get pointer to udevfd */
                                /* p points to (int *) */
#define AUX_GET_CDEVFD  10      /* get pointer to cdevfd */
                                /* p points to (int *) */
#define AUX_GET_ENVIRON 11      /* get pointer to environ */
                                /* p points to (char ***) */
#define AUX_LOGOUT 	12      /* call routine to logout */
				/* p points to integer flag */
				/*    - set to nonzero to clear screen */
#define	AUX_SWITCH	13	/* process context switch */
#define	AUX_GETTASK	14	/* get a/ux task descriptor */
#define	AUX_GETANYEVENT	15	/* get any event from the event queue */
#define	AUX_SETAUXMASK	16	/* a/ux events should be returned by
				   GetOSEvent and OSEventAvail,
				   p is an int */
#define	AUX_STOPTIMER	17	/* turn off the vbl and time mgr interrupt */
#define	AUX_STARTTIMER	18	/* turn on the vbl and time mgr interrupt */
#define	AUX_SETTIMEOUT	19	/* set the timeout value for GetOSEvent */
#define	AUX_SETBOUNDS	20	/* set the mouse moved region for GetOSEvent */
#define	AUX_KILL	21	/* kill the specified process, p contains a pid */
#define	AUX_FORKEXEC	22	/* fork and exec a new coff binary */
#define	AUX_REG_SIGIO	23	/* register a function and mask for SIGIO muxing */
#define	AUX_UNREG_SIGIO	24	/* unregister a function for SIGIO muxing */
#define AUX_VFS_VREFNUM 25	/* return the VREFNUM used for "/" */
#define AUX_CLEANFS     26	/* clean fs shutdown for Login */
#define	AUX_TRIM_CACHE	27	/* dispatch to compact the File Manager cache */
#define	AUX_ID_TO_PATH	28	/* translate a CNID to a *valid* UNIX path */
#define	AUX_FS_FREE_SPACE 29	/* get the # of free bytes for a filesystem */
#define AUX_CHECK_TB_LAUNCH 30  /* check if OK to launch the file */
#define	AUX_FS_USED_SPACE 31	/* get the # of used bytes for a filesystem */
#define	AUX_SAME_FS	32	/* returns true if 2 dirIDs on same FS */
#define AUX_MNT_CD	33	/* try to mount any available CD-ROMs */
#define AUX_RESTART_DLOG 34	/* post restart dialog but don't restart */
	/* p points to integer flag - set to nonzero to require root */
	/*   password to be entered even if already root (used by Login) */
#define AUX_SHUTDOWN_DLOG 35	/* post shutdown dialog but don't shutdown */
	/* p points to integer flag - set to nonzero to require root */
	/*   password to be entered even if already root (used by Login) */

                                /* For 7.0 and beyond: */
#define AUX_SECONDARY_INIT 36   /* Perform secondary initializations. */
#define AUX_COFF_FSSPEC 37      /* Get fsspec for a coff binary, p is taskid */
#define AUX_HOME_DIR 38		/* Get the cnid of the home directory */
#define AUX_GETUID 39		/* Get the effective user id */	
#define AUX_POST_EVTREC  40	/* post an entire event record */
#define AUX_FSTYPE  41		/* return the UNIX file sys type for a dir */
#define AUX_MAP_ID 42		/* moral equiv to PBMapID() */
#define AUX_MAP_NAME 43		/* moral equiv to PBMapName() */
#define AUX_SAME_DIRS 44	/* returns TRUE if 2 dirs stat equal */
#define AUX_SYNC_DIR 45		/* Used to inform the File Mgr when to */
				/* check dirs for open folders. */
#define AUX_BEGINAPPLICATION 46 /* Start up application */
#define AUX_ATTACHTIMER 47
#define AUX_GET_PHYSMEM 48	/* get physical memory */
#define AUX_ENABLE_COFF_LAUNCH 50/* launch of hybrid app's enabled? */

#define	AUX_VOL_FREE_SPACE  51	/* get the # of free bytes for a volume given a refnum */
#define AUX_GET_PATH_ID     52
#define AUX_GET_PATH_INFO   53
#define AUX_FREE_PATH_ID    54
#define AUX_GET_FCB_ADDR    55  /* map file refnum to FCB pointer */
#define AUX_GET_CACHE_INFO  56  /* return stats on unix file cache */
#define AUX_CLR_CACHE_INFO  57  /* clear unix file cache stats */
#define AUX_GET_NFILES      58  /* return max number of files that can be opened */
#define AUX_GET_BUF_CACHE   59  /* return info on buffer cache & avail mem */
#define AUX_SET_BUF_CACHE   60	/* set NBUF parameter for next reboot */
#define AUX_GET_MNT_DIRIDS  61  /* fetch the dirIDs of all local mount points */

#define MAX_SELECTOR    AUX_GET_MNT_DIRIDS

typedef struct	
{
	EventRecord		event;
	long			timeout;
	RgnHandle		mouseBounds;
	Boolean			pullIt;
	Boolean			found;
	short			mask;
} GetAnyEventRec, *GetAnyEventPtr;

#define	FIRST_AUX_EVENT		16

enum
{
	auxAttachEvent = FIRST_AUX_EVENT,
	auxExitEvent,
	auxSelectEvent
};

typedef struct
{
	short			vrefnum;
	char			*name;
	short			pid;
} ForkExecRec, *ForkExecPtr;


typedef struct
{
        int                     (*func)();
	int                     mask;
} AuxSigio;


typedef struct
{
	char		*path_buf;
	char		*fname;
	int		buflen;
	long		dir_id;
} IDToPathRec, *IDToPathPtr;

typedef struct
{
        char            *fname;
        long            dirid;
        short           vRefNum;
} TBLaunchRec, *TBLaunchPtr;

typedef struct
{
    short refnum;
    void *fcb_addr;
} AUX_FCBAddrRec, *AUX_FCBAddrPtr;


#define		OPEN_FOLDER		1
#define		CLOSE_FOLDER		2
#define		CHK_FOLDERS		3

typedef struct
{
	short		type;
	long		dirid;
} SyncDirRec, *SyncDirPtr;


typedef struct
{
        long            path_id;
        long            free;
	long            total;
} PathInfo;


  pascal long AUXDispatch(short selector, Ptr p)
    = 0xABF9;
  pascal short AUXCOFFLaunch(unsigned long regsize, unsigned long taskmode,
    Ptr procname, short vRefNum, unsigned long stackSize, short selector)
    = 0xA88F;

#endif /* __mac_aux_h */
