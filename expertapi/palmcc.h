/* palmcc.h
 * Header file for C-style interface to the PAL library
 * generated by apiwrapper for GAMS Version 43.0.0
 *
 * GAMS - Loading mechanism for GAMS Expert-Level APIs
 *
 * Copyright (c) 2016-2023 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2016-2023 GAMS Development Corp. <support@gams.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#if ! defined(_PALCC_H_)
#     define  _PALCC_H_

#define PALAPIVERSION 5

enum palModelSizeLimits {
  palLDemo       = 2000,
  palNLDemo      = 1000,
  palLCommunity  = 5000,
  palNLCommunity = 2500  };
#if defined(_WIN32) && defined(__GNUC__)
# include <stdio.h>
#endif


#if defined(_WIN32)
# define PAL_CALLCONV __stdcall
#else
# define PAL_CALLCONV
#endif
#if defined(_WIN32)
typedef __int64 INT64;
#else
typedef signed long int INT64;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct palRec;
typedef struct palRec *palHandle_t;

typedef int (*palErrorCallback_t) (int ErrCount, const char *msg);

/* headers for "wrapper" routines implemented in C */
/** palGetReady: load library
 *  @return false on failure to load library, true on success 
 */
int palGetReady  (char *msgBuf, int msgBufLen);

/** palGetReadyD: load library from the speicified directory
 * @return false on failure to load library, true on success 
 */
int palGetReadyD (const char *dirName, char *msgBuf, int msgBufLen);

/** palGetReadyL: load library from the specified library name
 *  @return false on failure to load library, true on success 
 */
int palGetReadyL (const char *libName, char *msgBuf, int msgBufLen);

/** palCreate: load library and create pal object handle 
 *  @return false on failure to load library, true on success 
 */
int palCreate    (palHandle_t *ppal, char *msgBuf, int msgBufLen);

/** palCreateD: load library from the specified directory and create pal object handle
 * @return false on failure to load library, true on success 
 */
int palCreateD   (palHandle_t *ppal, const char *dirName, char *msgBuf, int msgBufLen);

/** palCreate: load library from the specified library name and create pal object handle
 * @return false on failure to load library, true on success 
 */
int palCreateL   (palHandle_t *ppal, const char *libName, char *msgBuf, int msgBufLen);

/** palFree: free pal object handle 
 * @return false on failure, true on success 
 */
int palFree      (palHandle_t *ppal);

/** Check if library has been loaded
 * @return false on failure, true on success 
 */
int palLibraryLoaded(void);

/** Check if library has been unloaded
 * @return false on failure, true on success 
 */
int palLibraryUnload(void);

/** Check if API and library have the same version, Library needs to be initialized before calling this.
 * @return true  (1) on success,
 *         false (0) on failure.
 */
int  palCorrectLibraryVersion(char *msgBuf, int msgBufLen);

int  palGetScreenIndicator   (void);
void palSetScreenIndicator   (int scrind);
int  palGetExceptionIndicator(void);
void palSetExceptionIndicator(int excind);
int  palGetExitIndicator     (void);
void palSetExitIndicator     (int extind);
palErrorCallback_t palGetErrorCallback(void);
void palSetErrorCallback(palErrorCallback_t func);
int  palGetAPIErrorCount     (void);
void palSetAPIErrorCount     (int ecnt);

void palErrorHandling(const char *msg);
void palInitMutexes(void);
void palFiniMutexes(void);

#if defined(PAL_MAIN)    /* we must define some things only once */
# define PAL_FUNCPTR(NAME)  NAME##_t NAME = NULL
#else
# define PAL_FUNCPTR(NAME)  extern NAME##_t NAME
#endif

/* Prototypes for Dummy Functions */
void  PAL_CALLCONV d_palSetAuditLine (palHandle_t ppal, const char *auditline);
void  PAL_CALLCONV d_palSetSystemName (palHandle_t ppal, const char *sname);
int  PAL_CALLCONV d_palAuditRun (palHandle_t ppal);
char * PAL_CALLCONV d_palGetAuditLine (palHandle_t ppal, char *buf);
char * PAL_CALLCONV d_palGetCpr (palHandle_t ppal, char *buf);
int  PAL_CALLCONV d_palGetVer (palHandle_t ppal);
char * PAL_CALLCONV d_palGetRel (palHandle_t ppal, char *buf);
char * PAL_CALLCONV d_palGetGold (palHandle_t ppal, char *buf);
char * PAL_CALLCONV d_palGetCod (palHandle_t ppal, char *buf);
char * PAL_CALLCONV d_palGetHdr (palHandle_t ppal, char *buf);
int  PAL_CALLCONV d_palGetJul (palHandle_t ppal);
char * PAL_CALLCONV d_palGetLicDat (palHandle_t ppal, char *buf);
char * PAL_CALLCONV d_palGetBldCod (palHandle_t ppal, char *buf);
char * PAL_CALLCONV d_palGetRelDat (palHandle_t ppal, char *buf);
char * PAL_CALLCONV d_palGetRevision (palHandle_t ppal, char *buf);
int  PAL_CALLCONV d_palIsBeta (palHandle_t ppal);
int  PAL_CALLCONV d_palIsAlfa (palHandle_t ppal);
int  PAL_CALLCONV d_palIsAlpha (palHandle_t ppal);
int  PAL_CALLCONV d_palGetToday (palHandle_t ppal);
int  PAL_CALLCONV d_palGetJulianDays (palHandle_t ppal, const char *s);
void  PAL_CALLCONV d_palAuditFields (palHandle_t ppal, const char *auditline, char *v1, char *v2, char *v3);
char * PAL_CALLCONV d_palGetShortAuditLine (palHandle_t ppal, char *buf);
int  PAL_CALLCONV d_palLicenseReadU (palHandle_t ppal, const char *filename, char *msg, int *rc);
void  PAL_CALLCONV d_palLicenseRegisterGAMS (palHandle_t ppal, int linenr, const char *liceline);
char * PAL_CALLCONV d_palLicenseGetLLine (palHandle_t ppal, int linenr, char *buf);
void  PAL_CALLCONV d_palLicenseRegisterGAMSDone (palHandle_t ppal);
void  PAL_CALLCONV d_palLicenseRegisterSystem (palHandle_t ppal, int numcodes, const char *codes, int magicnum);
int  PAL_CALLCONV d_palLicenseValidation (palHandle_t ppal);
int  PAL_CALLCONV d_palLicenseSolverCheck (palHandle_t ppal, const char *codes);
int  PAL_CALLCONV d_palLicenseSolverCheckSizes (palHandle_t ppal, const char *codes, int M, int N, int NZ, int NLNZ, int NDisc);
int  PAL_CALLCONV d_palLicenseSolverCheckSizes64 (palHandle_t ppal, const char *codes, int M, int N, INT64 NZ, INT64 NLNZ, int NDisc);
int  PAL_CALLCONV d_palLicenseCheck (palHandle_t ppal, int M, int N, int NZ, int NLNZ, int NDisc);
int  PAL_CALLCONV d_palLicenseCheck64 (palHandle_t ppal, int M, int N, INT64 NZ, INT64 NLNZ, int NDisc);
int  PAL_CALLCONV d_palLicenseGetMessage (palHandle_t ppal, char *msg, int msg_i);
int  PAL_CALLCONV d_palPrintDemoMessage (palHandle_t ppal, char *msg, int msg_i);
int  PAL_CALLCONV d_palLicenseIsGAMSCheckout (palHandle_t ppal);
int  PAL_CALLCONV d_palLicenseIsAcademic (palHandle_t ppal);
int  PAL_CALLCONV d_palLicenseCheckSubSys (palHandle_t ppal, const char *codes);
int  PAL_CALLCONV d_palLicenseCheckSubX (palHandle_t ppal, const char *sname, const char *codes, int *daysLeft);
char * PAL_CALLCONV d_palLicenseGetPlatform (palHandle_t ppal, char *buf);
int  PAL_CALLCONV d_palDataDirs (palHandle_t ppal, char *buf, int bufsize, int *numdirs, int offset[], int offsetsize, const char *sysdir);
int  PAL_CALLCONV d_palConfigDirs (palHandle_t ppal, char *buf, int bufsize, int *numdirs, int offset[], int offsetsize, const char *sysdir);
int  PAL_CALLCONV d_palLicenseLevel (palHandle_t ppal);
void PAL_CALLCONV d_palLicenseLevelSet (palHandle_t ppal, const int x);


typedef void  (PAL_CALLCONV *palSetAuditLine_t) (palHandle_t ppal, const char *auditline);
/** Sets audit string
 *
 * @param ppal pal object handle
 * @param auditline audit line
 */
PAL_FUNCPTR(palSetAuditLine);

typedef void  (PAL_CALLCONV *palSetSystemName_t) (palHandle_t ppal, const char *sname);
/** Sets system name and sets up a generic audit string
 *
 * @param ppal pal object handle
 * @param sname name of solver/subsystem
 */
PAL_FUNCPTR(palSetSystemName);

typedef int  (PAL_CALLCONV *palAuditRun_t) (palHandle_t ppal);
/** Indicator for an audit run
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palAuditRun);

typedef char * (PAL_CALLCONV *palGetAuditLine_t) (palHandle_t ppal, char *buf);
/** Gets audit string
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetAuditLine);

typedef char * (PAL_CALLCONV *palGetCpr_t) (palHandle_t ppal, char *buf);
/** Gets copyright string
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetCpr);

typedef int  (PAL_CALLCONV *palGetVer_t) (palHandle_t ppal);
/** Major and Minor number combined as an int like 239
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetVer);

typedef char * (PAL_CALLCONV *palGetRel_t) (palHandle_t ppal, char *buf);
/** Gets Major.Minor like 23.9
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetRel);

typedef char * (PAL_CALLCONV *palGetGold_t) (palHandle_t ppal, char *buf);
/** Gets Maintenance number like 2 for 23.9.2
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetGold);

typedef char * (PAL_CALLCONV *palGetCod_t) (palHandle_t ppal, char *buf);
/** Gets Platform code like WIN
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetCod);

typedef char * (PAL_CALLCONV *palGetHdr_t) (palHandle_t ppal, char *buf);
/** Gets Platform text like x86/MS Windows
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetHdr);

typedef int  (PAL_CALLCONV *palGetJul_t) (palHandle_t ppal);
/** Get license check date as Julian number
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetJul);

typedef char * (PAL_CALLCONV *palGetLicDat_t) (palHandle_t ppal, char *buf);
/** Gets build date like "Sep 19, 2012"
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetLicDat);

typedef char * (PAL_CALLCONV *palGetBldCod_t) (palHandle_t ppal, char *buf);
/** Gets build code like LEG
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetBldCod);

typedef char * (PAL_CALLCONV *palGetRelDat_t) (palHandle_t ppal, char *buf);
/** Gets license check date like "Dec  3, 2021  (ALPHA)" or "Dec  3, 2021        "
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetRelDat);

typedef char * (PAL_CALLCONV *palGetRevision_t) (palHandle_t ppal, char *buf);
/** Gets global SVN revision number like r123456
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetRevision);

typedef int  (PAL_CALLCONV *palIsBeta_t) (palHandle_t ppal);
/** Indicator for an beta system
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palIsBeta);

typedef int  (PAL_CALLCONV *palIsAlfa_t) (palHandle_t ppal);
/** Indicator for an alpha system (deprecated)
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palIsAlfa);

typedef int  (PAL_CALLCONV *palIsAlpha_t) (palHandle_t ppal);
/** Indicator for an alpha system
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palIsAlpha);

typedef int  (PAL_CALLCONV *palGetToday_t) (palHandle_t ppal);
/** Get todays date as Julian number
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetToday);

typedef int  (PAL_CALLCONV *palGetJulianDays_t) (palHandle_t ppal, const char *s);
/** Get julian date from the string yymmdd
 *
 * @param ppal pal object handle
 * @param s 
 */
PAL_FUNCPTR(palGetJulianDays);

typedef void  (PAL_CALLCONV *palAuditFields_t) (palHandle_t ppal, const char *auditline, char *v1, char *v2, char *v3);
/** Chops audit line apart
 *
 * @param ppal pal object handle
 * @param auditline audit line
 * @param v1 product part of audit line
 * @param v2 distribution info of audit line
 * @param v3 rest of audit line
 */
PAL_FUNCPTR(palAuditFields);

typedef char * (PAL_CALLCONV *palGetShortAuditLine_t) (palHandle_t ppal, char *buf);
/** Gets a short audit line
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palGetShortAuditLine);

typedef int  (PAL_CALLCONV *palLicenseReadU_t) (palHandle_t ppal, const char *filename, char *msg, int *rc);
/** Reads license file in various line formats
 *
 * @param ppal pal object handle
 * @param filename name of the license file to read
 * @param msg message
 * @param rc return code
 */
PAL_FUNCPTR(palLicenseReadU);

typedef void  (PAL_CALLCONV *palLicenseRegisterGAMS_t) (palHandle_t ppal, int linenr, const char *liceline);
/** Registers a line of the license file
 *
 * @param ppal pal object handle
 * @param linenr license file line number
 * @param liceline license file line
 */
PAL_FUNCPTR(palLicenseRegisterGAMS);

typedef char * (PAL_CALLCONV *palLicenseGetLLine_t) (palHandle_t ppal, int linenr, char *buf);
/** Retrieve a line of the license file
 *
 * @param ppal pal object handle
 * @param linenr license file line number
 */
PAL_FUNCPTR(palLicenseGetLLine);

typedef void  (PAL_CALLCONV *palLicenseRegisterGAMSDone_t) (palHandle_t ppal);
/** Completion of license registration
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palLicenseRegisterGAMSDone);

typedef void  (PAL_CALLCONV *palLicenseRegisterSystem_t) (palHandle_t ppal, int numcodes, const char *codes, int magicnum);
/** Registers the system to be licensed
 *
 * @param ppal pal object handle
 * @param numcodes number of license codes for system
 * @param codes license codes for system
 * @param magicnum magic number
 */
PAL_FUNCPTR(palLicenseRegisterSystem);

typedef int  (PAL_CALLCONV *palLicenseValidation_t) (palHandle_t ppal);
/** Validates license and returns true on failure
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palLicenseValidation);

typedef int  (PAL_CALLCONV *palLicenseSolverCheck_t) (palHandle_t ppal, const char *codes);
/** Checks if system with codes is licensed and returns true on failure uses registered size
 *
 * @param ppal pal object handle
 * @param codes license codes for system
 */
PAL_FUNCPTR(palLicenseSolverCheck);

typedef int  (PAL_CALLCONV *palLicenseSolverCheckSizes_t) (palHandle_t ppal, const char *codes, int M, int N, int NZ, int NLNZ, int NDisc);
/** Checks if system with codes is licensed and returns true on failure uses sizes from arguments
 *
 * @param ppal pal object handle
 * @param codes license codes for system
 * @param M number of equations
 * @param N number of variables
 * @param NZ number of non-zeros
 * @param NLNZ number of non-linear non-zeros
 * @param NDisc number of discrete variables
 */
PAL_FUNCPTR(palLicenseSolverCheckSizes);

typedef int  (PAL_CALLCONV *palLicenseSolverCheckSizes64_t) (palHandle_t ppal, const char *codes, int M, int N, INT64 NZ, INT64 NLNZ, int NDisc);
/** Checks if system with codes is licensed and returns true on failure uses sizes from arguments
 *
 * @param ppal pal object handle
 * @param codes license codes for system
 * @param M number of equations
 * @param N number of variables
 * @param NZ number of non-zeros
 * @param NLNZ number of non-linear non-zeros
 * @param NDisc number of discrete variables
 */
PAL_FUNCPTR(palLicenseSolverCheckSizes64);

typedef int  (PAL_CALLCONV *palLicenseCheck_t) (palHandle_t ppal, int M, int N, int NZ, int NLNZ, int NDisc);
/** Checks if system is licensed and returns true on failure
 *
 * @param ppal pal object handle
 * @param M number of equations
 * @param N number of variables
 * @param NZ number of non-zeros
 * @param NLNZ number of non-linear non-zeros
 * @param NDisc number of discrete variables
 */
PAL_FUNCPTR(palLicenseCheck);

typedef int  (PAL_CALLCONV *palLicenseCheck64_t) (palHandle_t ppal, int M, int N, INT64 NZ, INT64 NLNZ, int NDisc);
/** Checks if system is licensed and returns true on failure
 *
 * @param ppal pal object handle
 * @param M number of equations
 * @param N number of variables
 * @param NZ number of non-zeros
 * @param NLNZ number of non-linear non-zeros
 * @param NDisc number of discrete variables
 */
PAL_FUNCPTR(palLicenseCheck64);

typedef int  (PAL_CALLCONV *palLicenseGetMessage_t) (palHandle_t ppal, char *msg, int msg_i);
/** Retrieves message from license check
 *
 * @param ppal pal object handle
 * @param msg message
 */
PAL_FUNCPTR(palLicenseGetMessage);

typedef int  (PAL_CALLCONV *palPrintDemoMessage_t) (palHandle_t ppal, char *msg, int msg_i);
/** Retrieves message about demo usage. Return indicates if message should be printed (true means print)
 *
 * @param ppal pal object handle
 * @param msg message
 */
PAL_FUNCPTR(palPrintDemoMessage);

typedef int  (PAL_CALLCONV *palLicenseIsGAMSCheckout_t) (palHandle_t ppal);
/** Indicator if model fits in free GAMS limits (demo or community based on the actual license), returns true if that is the case, and false if the model is too big and a check for a license code is in order
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palLicenseIsGAMSCheckout);

typedef int  (PAL_CALLCONV *palLicenseIsAcademic_t) (palHandle_t ppal);
/** Indicator if license is academic
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palLicenseIsAcademic);

typedef int  (PAL_CALLCONV *palLicenseCheckSubSys_t) (palHandle_t ppal, const char *codes);
/** Indicator if subsystem is licensed (true if not licenses)
 *
 * @param ppal pal object handle
 * @param codes license codes for system
 */
PAL_FUNCPTR(palLicenseCheckSubSys);

typedef int  (PAL_CALLCONV *palLicenseCheckSubX_t) (palHandle_t ppal, const char *sname, const char *codes, int *daysLeft);
/** Indicator if subsystem is licensed (true if not licenses, daysLeft will be >/< 0 for eval license)
 *
 * @param ppal pal object handle
 * @param sname name of solver/subsystem
 * @param codes license codes for system
 * @param daysLeft remaining days of eval license (negative if expired)
 */
PAL_FUNCPTR(palLicenseCheckSubX);

typedef char * (PAL_CALLCONV *palLicenseGetPlatform_t) (palHandle_t ppal, char *buf);
/** Get platform code of license
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palLicenseGetPlatform);

typedef int  (PAL_CALLCONV *palDataDirs_t) (palHandle_t ppal, char *buf, int bufsize, int *numdirs, int offset[], int offsetsize, const char *sysdir);
/** Returns data dirs in buf
 *
 * @param ppal pal object handle
 * @param buf buffer to return a number of directories
 * @param bufsize size of buf
 * @param numdirs number of directories returned
 * @param offset vector of offsets into buf to access the individual directories
 * @param offsetsize size of offset vector
 * @param sysdir GAMS system directory
 */
PAL_FUNCPTR(palDataDirs);

typedef int  (PAL_CALLCONV *palConfigDirs_t) (palHandle_t ppal, char *buf, int bufsize, int *numdirs, int offset[], int offsetsize, const char *sysdir);
/** Returns config dirs in buf
 *
 * @param ppal pal object handle
 * @param buf buffer to return a number of directories
 * @param bufsize size of buf
 * @param numdirs number of directories returned
 * @param offset vector of offsets into buf to access the individual directories
 * @param offsetsize size of offset vector
 * @param sysdir GAMS system directory
 */
PAL_FUNCPTR(palConfigDirs);

typedef int  (PAL_CALLCONV *palLicenseLevel_t) (palHandle_t ppal);
/** Access the license level
 *
 * @param ppal pal object handle
 */
PAL_FUNCPTR(palLicenseLevel);

typedef void (PAL_CALLCONV *palLicenseLevelSet_t) (palHandle_t ppal, const int x);
PAL_FUNCPTR(palLicenseLevelSet);

#if defined(__cplusplus)
}
#endif
#endif /* #if ! defined(_PALCC_H_) */

