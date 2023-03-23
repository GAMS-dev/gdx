/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2023 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2023 GAMS Development Corp. <support@gams.com>
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

#pragma once

#include "gxfile.h"

namespace gdx
{

// @details
//    Class for reading and writing gdx files
class TGXFileObj
{
public:
   explicit TGXFileObj( std::string &ErrMsg );
   ~TGXFileObj();

   /**
@brief Open a new GDX file for output. 
@details Uses the environment variable GDXCOMPRESS to set compression argument for gdxOpenWriteEx.
@param FileName File name of the GDX file to be created.
@param Producer Name of program that creates the GDX file.
@param ErrNr Returns an error code or zero if there is no error.
@return Returns non-zero if the file can be opened; zero otherwise.
@see gdxOpenRead, gdxOpenWriteEx, Destroy
*/
   int gdxOpenWrite( const char *FileName, const char *Producer, int &ErrNr );

   /**
@brief Create a gdx file for writing
@details Open a new gdx file for output. If a file extension is not
supplied, the extension '.gdx' will be used. The return code is
a system dependent I/O error.
@param FileName File name of the gdx file to be created
@param Producer Name of program that creates the gdx file
@param Compr Zero for no compression; non-zero uses compression if available
@param ErrNr Returns an error code or zero if there is no error
@attention when writing compressed, set the AutoConvert flag to zero so the file is not uncompressed after the Close; see gdxAutoConvert
@return Returns non-zero if the file can be opened; zero otherwise

@code{.pas}
var
ErrNr: integer;
PGX : PGXFile;
Msg : ShortString;
begin
if not gdxGetReady(Msg)
then
begin
WriteLn('Cannot load GDX library, msg: ', Msg);
exit;
end;
gdxOpenWriteEx(PGX,'c:\mydata\file1.gdx','Examples', 1, ErrCode);
gdxAutoConvert(PGX, 0);
if ErrCode <> 0
then
[ ... ]
@endcode

@see gdxOpenRead(), gdxOpenWrite(), gdxAutoConvert(), Destroy()
*/
   int gdxOpenWriteEx( const char *FileName, const char *Producer, int Compr, int &ErrNr );


   /**
@brief  Start writing a new symbol in string mode
@param SyId Name of the symbol
@param ExplTxt Explanatory text for the symbol
@param Dim Dimension of the symbol
@param Typ Type of the symbol
@param UserInfo See gdxDataWriteRawStart for more information
@returns  Non-zero if the operation is possible, zero otherwise
@see  gdxDataWriteStr, gdxDataWriteDone
*/
   int gdxDataWriteStrStart( const char *SyId, const char *ExplTxt, int Dim, int Typ, int UserInfo );

   /**
@brief  Write a data element in string mode
@details
  When writing data using string elements, each string element is added
  to the internal unique element table and assigned an index.
  Writing using strings does not add the unique elements to the
  user mapped space.
  Each element string must follow the GAMS rules for unique elements.
@param KeyStr The index for this element using strings for the unique elements
@param Values The values for this element
@returns Non-zero if the operation is possible, zero otherwise
@see  gdxDataWriteMapStart, gdxDataWriteDone
*/
   int gdxDataWriteStr( const char **KeyStr, const double *Values );

   /**
@brief  Finish a write operation
@returns  Non-zero if the operation is possible, zero otherwise
@see gdxDataErrorCount, gdxDataWriteRawStart, gdxDataWriteMapStart, gdxDataWriteStrStart,
*/
   int gdxDataWriteDone();

   /**
@brief  Start registering unique elements in mapped mode
@returns  Non-zero if the operation is possible, zero otherwise
@see  gdxUELRegisterMap, gdxUELRegisterDone
*/
   int gdxUELRegisterMapStart();

   /**
@brief  Register an unique elements in mapped mode
@details
 Register a unique element in mapped space; UMap is the user assigned
 index for the element. Registering an element a second time is not considered
 an error as long as the same UMap is used. Assigning different elements
 with the same UMap value is an error.
  A unique element
  must follow the GAMS rules when it contains quote characters.
@param  UMap User index number to be assigned to the unique element
@param  Uel String for unique element
@returns  Non-zero if the operation is possible, zero otherwise
@see  gdxUELRegisterMapStart, gdxUELRegisterDone
*/
   int gdxUELRegisterMap( int UMap, const char *Uel );

   /**
@brief  Close a gdx file
@details  Close a gdx file that was previously opened for reading or writing.
  Before the file is closed, any pending write operations will be
  finished. To free the gdx object, call gdxFree.
@returns  Returns the value of gdxGetLastError
@see  gdxOpenRead, gdxOpenWrite
*/
   int gdxClose();

   /**
@brief  Reset the internal values for special values
@return Always non-zero
@see  gdxSetSpecialValues, gdxGetSpecialValues
*/
   int gdxResetSpecialValues();

   /**
@brief  Returns the text for a given error number
@param   ErrNr Error number
@param   ErrMsg Contains error text after return
@return  Always returns non-zero
@see  gdxGetLastError
*/
   static int gdxErrorStr( int ErrNr, char *ErrMsg );

   /**
@brief  Open a gdx file for reading
@details
  Open an existing gdx file for input. If a file extension is not
  supplied, the extension '.gdx' will be used. The return code is
  a system dependent I/O error. If the file was found, but is not
  a valid gdx file, the function GetLastError can be used to handle
  these type of errors.
@param  FileName file name of the gdx file to be opened
@param  ErrNr Returns an error code or zero if there is no error
@return Returns non-zero if the file can be opened; zero otherwise
@see  gdxOpenWrite, Destroy, gdxGetLastError

@code
  var
     ErrNr: integer;
     PGX  : PGXFile;
  begin
  gdxOpenRead(PGX,'c:\\mydata\\file1.gdx', ErrNr);
  if ErrNr <> 0
  then
     begin
     [...]
@endcode
*/
   int gdxOpenRead( const char *FileName, int &ErrNr );

   /**
@brief  Return strings for file version and file producer
@param  FileStr Version string
@param  ProduceStr Producer string
@return
  Always non-zero
@see  gdxOpenWrite, gdxOpenWriteEx
*/
   int gdxFileVersion( char *FileStr, char *ProduceStr ) const;

   /**
@brief  Find symbol by name
@details
  Search for a symbol by name; the search is not case sensitive.
  When the symbol is found, SyNr contains the symbol number and the
  function returns a non-zero integer. When the symbol is not found, the function
  returns zero.
@param  SyId Name of the symbol
@param  SyNr Symbol number
@return
  Non-zero if the symbol is found, zero otherwise.
@see  gdxSymbolInfo, gdxSymbolInfoX
*/
   int gdxFindSymbol( const char *SyId, int &SyNr );

   /**
@brief  Read the next record in string mode
@details
  Read the next record using strings for the unique elements. The
  reading should be initialized by calling DataReadStrStart
@param  KeyStr The index of the record as strings for the unique elements
@param  Values The data of the record
@param  DimFrst The first index position in KeyStr that changed
@return
  Non-zero if the operation is possible; return zero if the operation is not
  possible or if there is no more data
@see  gdxDataReadStrStart, gdxDataReadDone
*/
   int gdxDataReadStr( char **KeyStr, double *Values, int &DimFrst );

   /**
@brief  Finish reading of a symbol in any mode(raw, mapped, string)
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataReadRawStart, gdxDataReadMapStart, gdxDataReadStrStart
*/
   int gdxDataReadDone();

   /**
@brief Returns information about a symbol

@param  SyNr The symbol number (range 0..NrSymbols); return universe info when SyNr = 0
@param  SyId Name of the symbol
@param  Dim Dimension of the symbol
@param  Typ Symbol type
@return
  Zero if the symbol number is not in the correct range, non-zero otherwise
@see  gdxSystemInfo, gdxSymbolInfoX, gdxSymbolDim, gdxFindSymbol
*/
   int gdxSymbolInfo( int SyNr, char *SyId, int &Dim, int &Typ );


   /**
@brief  Initialize the reading of a symbol in string mode
@details
  Reading data using strings is the simplest way to read data.
  Every record read using DataReadStr will return the strings
  for the unique elements. Internal mapping is not affected by
  this function.
@param  SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
@param  NrRecs The number of records available for reading
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataReadStr, gdxDataReadRawStart, gdxDataReadMapStart, gdxDataReadDone

@code
  if gdxDataReadStrStart(PGX,1,NrRecs)
  then
     begin
     while gdxDataReadStr(PGX,Uels,Vals)
     do [...]
     gdxDataReadDone(PGX)
     end;
@endcode
*/
   int gdxDataReadStrStart( int SyNr, int &NrRecs );


   /**
@brief
  Add an alias for a set to the symbol table
@details
   One of the two identifiers has to be a known set, an alias or * (universe);
   the other identifier is used as the new alias for the given set.
   The function gdxSymbolInfoX can be used to retrieve the set or alias
      associated with the identifier; it is returned as the UserInfo parameter.
@param   Id1 set identifier
@param   Id2 set identifier
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxSymbolSetDomain
*/
   int gdxAddAlias( const char *Id1, const char *Id2 );


   /**
@brief  Register a string in the string table
@details
 Register a string in the string table and return the integer number assigned to this string.
 The integer value can be used to set the associated text of a set element.
 The string must follow the GAMS syntax rules for explanatory text.
@param  Txt The string to be registered
@param  TxtNr The index number assigned to this string
@return
  Non-zero if the operation is possible, zero otherwise
@see gdxGetElemText, gdxSetTextNodeNr
*/
   int gdxAddSetText( const char *Txt, int &TxtNr );


   /**
@brief  The number of error records
@details
  After a write operation is finished (gdxDataWriteDone), the data
  is sorted and written to the gdx file. If there are duplicate records,
  the first record is written to the file and the duplicates are
  added to the error list.
  <P>
  When reading data using a filtered read operation, data records that were
  filtered out because an index is not in the user index space or not in a
  filter are added the error list.
@return
  The number of error records available.
@see  gdxDataErrorRecord
*/
   [[nodiscard]] int gdxDataErrorCount() const;


   /**
@brief  Retrieve an error record
@param  RecNr The number of the record to be retrieved, range = 1..NrErrorRecords
@param  KeyInt Index for the record
@param  Values Values for the record
@return
  Non-zero if the record number is valid, zero otherwise
@see  gdxDataErrorCount
*/
   int gdxDataErrorRecord( int RecNr, int *KeyInt, double *Values );


   /**
@brief  Retrieve an error record
@param  RecNr The number of the record to be retrieved, range = 1..NrErrorRecords
@param  KeyInt Index for the record, negative uel indicates domain violation for filtered/strict read
@param  Values Values for the record
@return
  Non-zero if the record number is valid, zero otherwise
@see  gdxDataErrorCount
*/
   int gdxDataErrorRecordX( int RecNr, int *KeyInt, double *Values );


   /**
@brief  Read the next record in raw mode
@param  KeyInt The index of the record
@param  Values The data of the record
@param  DimFrst The first index position in KeyInt that changed
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataReadRawStart, gdxDataReadDone
*/
   int gdxDataReadRaw( int *KeyInt, double *Values, int &DimFrst );


   /**
@brief  Initialize the reading of a symbol in raw mode
@param  SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
@param  NrRecs The number of records available for reading
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataReadRaw, gdxDataReadMapStart, gdxDataReadStrStart, gdxDataReadDone
*/
   int gdxDataReadRawStart( int SyNr, int &NrRecs );


   /**
@brief  Write a data element in raw mode
@details
  When writing data in raw mode, the index space used is based on the
  internal index space. The indices used are in the range 1..NrUels but this is not enforced.
  Before we can write in raw mode, the unique elements (strings) should
  be registered first.
  <P>
  When writing raw, it assumed that the records are written in sorted order and
  that there are no duplicate records. Records that are not in sorted order or are
  duplicates will be added to the error list (see DataErrorCount and DataErrorRecord)
@param  KeyInt The index for this element
@param  Values The values for this element
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataWriteRawStart, gdxDataWriteDone
*/
   int gdxDataWriteRaw( const int *KeyInt, const double *Values );


   /**
@brief  Start writing a new symbol in raw mode
@param  SyId Name of the symbol
@param  ExplTxt Explanatory text for the symbol
@param  Dimen Dimension of the symbol
@param  Typ Type of the symbol
@param  UserInfo GAMS follows the following conventions:

| Type         | Value(s)                                                             |
| ------------ | -------------------------------------------------------------------- |
| Aliased Set  | The symbol number of the aliased set, or zero for the universe       |
| Set          | Zero                                                                 |
| Parameter    | Zero                                                                 |
| Variable     | The variable type: binary=1, integer=2, positive=3, negative=4, free=5, sos1=6, sos2=7, semicontinous=8, semiinteger=9      |
| Equation     | The equation type: eque=53, equg=54, equl=55, equn=56, equx=57, equc=58, equb=59      |


@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataWriteRaw, gdxDataWriteDone
*/
   int gdxDataWriteRawStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );


   /**
@brief Returns the number of errors
@return
 Total number of errors encountered
@see  gdxGetLastError
*/
   [[nodiscard]] int gdxErrorCount() const;


   /**
@brief  Retrieve the string and node number for an entry in the string table
@details
  Retrieve a string based on the string table index. When writing to a gdx file,
  this index is the value returned by calling gdxAddSetText. When reading a gdx file,
  the index is returned as the level value when reading a set.
  The Node number can be used as an index in a string table in the user space;
  the value is set by calling SetTextNodeNr. If the Node number was never assigned,
  it will be returned as zero.
@param  TxtNr String table index
@param  Txt Text found for the entry
@param  Node Node number found for the entry
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxAddSetText, gdxSetTextNodeNr

@code
[assumes we are reading using strings ...]
while gdxDataReadStr(PGX, Uels, Vals) <> 0
do begin
   for D := 1 to Dim
   do Write(Uels[D], '  ');
   indx := Round(Vals[vallevel]);
   if indx > 0
   then
      begin
      gdxGetElemText(indx, S, N);
      Write('txt = ', S, ' Node = ', N);
      end;
   WriteLn;
   end
@endcode
*/
   int gdxGetElemText( int TxtNr, char *Txt, int &Node );


   /**
@brief Return the last error
@details
 When an error is encountered, an error code is stored which can
 be retrieved with this function. If subsequent errors occur before
 this function is called, the first error code will be maintained.
 Calling this function will clear the last error stored.
@return
 The error number, or zero if there was no error
@see gdxErrorCount
*/
   int gdxGetLastError();


   /**
@brief  Retrieve the internal values for special values
@param Avals array of special values used for Eps, +Inf, -Inf, NA and Undef
@return
  Always non-zero
@see gdxResetSpecialValues, gdxSetSpecialValues
*/
   int gdxGetSpecialValues( double *Avals );


   /**
@brief  Set the internal values for special values
@param
  AVals array of special values to be used for Eps, +Inf, -Inf, NA and Undef
         Note that the values have to be unique
@return
  Non-zero if all values specified are unique, zero otherwise
@note Before calling this function, initialize the array of special values
  by calling gdxGetSpecialValues first
@see  gdxSetReadSpecialValues, gdxResetSpecialValues, gdxGetSpecialValues
*/
   int gdxSetSpecialValues( const double *AVals );


   /**
@brief
  Retrieve the domain of a symbol

@param SyNr The index number of the symbol, range 1..NrSymbols
@param DomainSyNrs array returning the set identifiers or *;
  DomainSyNrs[D] will contain the index number of the one dimensional
  set or alias used as the domain for index position D. A value of zero represents the universe ( * )
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxSymbolSetDomain, gdxSymbolGetDomainX
*/
   int gdxSymbolGetDomain( int SyNr, int *DomainSyNrs );


   /**
@brief
  Retrieve the domain of a symbol (using relaxed or domain information)

@param  SyNr The index number of the symbol, range 1..NrSymbols DomainIDs[D] will contain the strings as they were stored with the call gdxSymbolSetDomainX. If gdxSymbolSetDomainX was never called, but gdxSymbolSetDomain was called, that information will be used instead.
@return
  - 0: If operation was not possible (Bad SyNr)
  - 1: No domain information was available
  - 2: Data used was defined using gdxSymbolSetDomainX
  - 3: Data used was defined using gdxSymbolSetDomain
@see  gdxSymbolSetDomainX, gdxSymbolSetDomain
*/
   int gdxSymbolGetDomainX( int SyNr, char **DomainIDs );


   /**
@brief Returns Dimension of a symbol

@param  SyNr The symbol number (range 0..NrSymbols); return universe info when SyNr = 0
@return
  -1 if the symbol number is not in the correct range, the symbol dimension otherwise
@see  gdxSymbolInfo, gdxSymbolInfoX, gdxFindSymbol
*/
   int gdxSymbolDim( int SyNr );


   /**
@brief Returns additional information about a symbol

@param  SyNr The symbol number (range 0..NrSymbols); return universe info when SyNr = 0
@param  RecCnt Total number of records stored (unmapped); for the universe (SyNr = 0) this is the
     number of entries when the gdx file was openened for reading.
@param  UserInfo User field value; see gdxDataWriteRawStart for more information
@param  ExplTxt Explanatory text for the symbol
@return
  Zero if the symbol number is not in the correct range, non-zero otherwise
@see  gdxSystemInfo, gdxSymbolInfo, gdxFindSymbol
*/
   int gdxSymbolInfoX( int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt );


   /**
@brief
  Define the domain of a symbol
@details
  This function defines the domain for the symbol for which a write
    data operation just started using DataWriteRawStart, DataWriteMapStart or
    DataWriteStrStart. At this point the symbol and dimension is known,
    but no data has been written yet.
  Each identifier will be checked to be a one dimensional set or an alias.
  When a domain is specified, write operations will be domain checked; records
  violating the domain will be added the the internal error list (see DataErrorCount
  and DataErrorRecord.)
@param
  DomainIDs array of identifers or *
@return
  Non-zero if the operation is possible, zero otherwise

@see  gdxSymbolGetDomain
*/
   int gdxSymbolSetDomain( const char **DomainIDs );


   /**
@brief
  Define the domain of a symbol (relaxed version)
@details
  This function defines the relaxed domain information for the symbol SyNr.
  The identifiers will NOT be checked to be known one-dimensional sets, and
  no domain checking will be performed. This function can be called during or after
  the write operation.
  If domain checking is needed, use gdxSymbolSetDomain
@param
  DomainIDs array of identifers or *
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxSymbolSetDomain, gdxSymbolGetDomainX
*/
   int gdxSymbolSetDomainX( int SyNr, const char **DomainIDs );


   /**
@brief  Returns the number of symbols and unique elements

@param  SyCnt Number of symbols available in the gdx file
@param  UelCnt Number of unique elements stored in the gdx file
@return
  Returns a non-zero value
*/
   int gdxSystemInfo( int &SyCnt, int &UelCnt ) const;


   /**
@brief  Finish registration of unique elements
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxUELRegisterRawStart, gdxUELRegisterMapStart, gdxUELRegisterStrStart
*/
   int gdxUELRegisterDone();


   /**
@brief  Register an unique elements in raw mode
@details
 The unique element is registered in raw mode, i.e. the internally
 assigned integer index is determined by the system
 Can only be used while writing to a gdx file
@param
  Uel String for unique element
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxUELRegisterMap, gdxUELRegisterDone

*/
   int gdxUELRegisterRaw( const char *Uel );


   /**
@brief  Start registering unique elements in raw mode
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxUELRegisterRaw, gdxUELRegisterDone
*/
   int gdxUELRegisterRawStart();


   /**
@brief  Register a unique element in string mode
@details
 The unique element is registered in user mapped space. The returned
 index is the next higher value. Registering an element a second time
 is not considered an error and the same index position will be returned.
  A unique element must follow the GAMS rules when it contains quote characters.
@param  Uel String for unique element
@param  UelNr Index number assigned to this unique element in user space
@return
  Non-zero if the element was registered, zero otherwise.

@see  gdxUELRegisterStrStart, gdxUELRegisterDone
*/
   int gdxUELRegisterStr( const char *Uel, int &UelNr );


   /**
@brief  Start registering unique elements in string mode
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxUELRegisterStr, gdxUELRegisterDone
*/
   int gdxUELRegisterStrStart();


   /**
@brief  Get a unique element using an unmapped index

@param  UelNr Element number (unmapped) in the range 1..NrElem
@param  Uel String for unique element
@param  UelMap User mapping for this element or -1 if element was never mapped
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxUMUelInfo, gdxGetUEL
*/
   int gdxUMUelGet( int UelNr, char *Uel, int &UelMap );


   /**
@brief  Return information about the unique elements

@param  UelCnt Total number of unique elements (uels in gdx file + new registered uels)
@param  HighMap Highest user mapping index used
@return
  Always returns non-zero
@see gdxUMUelGet
*/
   int gdxUMUelInfo( int &UelCnt, int &HighMap ) const;


   /**
@brief
  Returns the dimension of the current active symbol
@details
   When reading or writing data, the dimension of the current active symbol
   is sometimes needed to convert arguments from strings to pchars etc.
@return
  Dimension of current active symbol
    */
   [[nodiscard]] int gdxCurrentDim() const;


   /**
@brief  Rename UEL OldName to NewName
@param  OldName Name of an existing UEL
@param  NewName New name for the UEL
@return Zero if the renaming was possible; non-zero is an error indicator
*/
   int gdxRenameUEL( const char *OldName, const char *NewName );


   /**
@brief  Open a gdx file for reading
@details
  Open an existing gdx file for input. If a file extension is not
  supplied, the extension '.gdx' will be used. The return code is
  a system dependent I/O error. If the file was found, but is not
  a valid gdx file, the function GetLastError can be used to handle
  these type of errors.
@param  FileName file name of the gdx file to be opened
@param  ReadMode bitmap skip reading sections: 0-bit: string (1 skip reading string)
@param  ErrNr Returns an error code or zero if there is no error
@return
  Returns non-zero if the file can be opened; zero otherwise
@see  gdxOpenWrite, Destroy, gdxGetLastError


@code
  var
     ErrNr: integer;
     PGX  : PGXFile;
  begin
  gdxOpenRead(PGX,'c:\\mydata\\file1.gdx', ErrNr);
  if ErrNr <> 0
  then
     begin
     [...]
@endcode
*/
   int gdxOpenReadEx( const char *FileName, int ReadMode, int &ErrNr );


   /**
@brief Get the string for a unique element using a mapped index
@details
 Retrieve the string for an unique element based on a mapped index number.
@param  uelNr Index number in user space (1..NrUserElem)
@param  Uel String for the unique element
@return
 Return non-zero if the index is in a valid range, zero otherwise

@see  gdxUMUelGet
*/
   int gdxGetUEL( int uelNr, char *Uel ) const;


   /**
@brief  Start writing a new symbol in mapped mode
@param  SyId Name of the symbol
@param  ExplTxt Explanatory text for the symbol
@param  Dimen Dimension of the symbol
@param  Typ Type of the symbol
@param  UserInfo See gdxDataWriteRawStart for more information
@return Non-zero if the operation is possible, zero otherwise
@see  gdxDataWriteMap, gdxDataWriteDone
*/
   int gdxDataWriteMapStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );


   /**
@brief  Write a data element in mapped mode
@param  KeyInt The index for this element using mapped values
@param  Values The values for this element
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataWriteMapStart, gdxDataWriteDone
*/
   int gdxDataWriteMap( const int *KeyInt, const double *Values );


   /**
@brief  Initialize the reading of a symbol in mapped mode
@param  SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
@param  NrRecs The number of records available for reading
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataReadMap, gdxDataReadRawStart, gdxDataReadStrStart, gdxDataReadDone
*/
   int gdxDataReadMapStart( int SyNr, int &NrRecs );


   /**
@brief  Read the next record in mapped mode
@param  RecNr Ignored (left in for backward compatibility)
@param  KeyInt The index of the record
@param  Values The data of the record
@param  DimFrst The first index position in KeyInt that changed
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxDataReadMapStart, gdxDataReadFilteredStart, gdxDataReadDone
*/
   int gdxDataReadMap( int RecNr, int *KeyInt, double *Values, int &DimFrst );

   enum class TraceLevels
   {
      trl_none,
      trl_errors,
      trl_some,
      trl_all
   };
   void SetTraceLevel( TraceLevels tl );

   // region Acronym handling
   /**
@brief
  Number of entries in the acronym table
@return
  The number of entries in the acronym table
@see   gdxAcronymSetInfo, gdxAcronymSetInfo
*/
   [[nodiscard]] int gdxAcronymCount() const;


   /**
@brief
  Retrieve acronym information from the acronym table

@param  N Index into acronym table; range from 1 to AcronymCount
@param  AName Name of the acronym
@param  Txt Explanatory text of the acronym
@param  AIndx  Index value of the acronym
@return
  Non-zero if the index into the acronym table is valid; false otherwise
@see   gdxAcronymSetInfo, gdxAcronymCount
*/
   int gdxAcronymGetInfo( int N, char *AName, char *Txt, int &AIndx ) const;


   /**
@brief
  Modify acronym information in the acronym table
@details
  When writing a gdx file, this function is used to provide the name of an acronym;
    in this case the Indx parameter must match.
  When reading a gdx file, this function
    is used to provide the acronym index, and the AName parameter must match.
@param  N Index into acronym table; range from 1 to AcronymCount
@param  AName Name of the acronym
@param  Txt Explanatory text of the acronym
@param  AIndx  Index value of the acronym
@return
  Non-zero if the index into the acronym table is valid; false otherwise
@see  gdxAcronymGetInfo, gdxAcronymCount

*/
   int gdxAcronymSetInfo( int N, const char *AName, const char *Txt, int AIndx );


   /**
@brief
  Returns the value of the NextAutoAcronym variable and sets the variable to nv
@details
   When we read from a gdx file and encounter an acronym that was not defined, we need to assign
   a new index for that acronym. The variable NextAutoAcronym is used for this purpose and is
   incremented for each new undefined acronym.
   When NextAutoAcronym has a value of zero, the default, the value is ignored and the original
   index as stored in the gdx file is used for the index.
@param   nv New value for NextAutoAcronym; a value of less than zero is ignored
@return
   Previous value of NextAutoAcronym

*/
   int gdxAcronymNextNr( int nv );


   /**
@brief
  Get information how acronym values are remapped
@details
  When reading gdx data, we need to map indices for acronyms used in the gdx file to
  indices used by the reading program. There is a problen when not all acronyms have been
  registered before reading the gdx data. We need to map an udefined index we read to a new value.
  The value of NextAutoAcronym is used for that.
@param  N Index into acronym table; range from 1 to AcronymCount
@param  orgIndx The Index used in the gdx file
@param  newIndx The Index returned when reading gdx data
@param  autoIndex non-zero if the newIndx was generated using the value of NextAutoAcronym
@return
  Non-zero if the index into the acronym table is valid; false otherwise
@see  gdxAcronymGetInfo, gdxAcronymCount, gdxAcronymNextNr
@details
  When reading gdx data, we need to map indices for acronyms used in the gdx file to
  indices used by the reading program. There is a problen when not all acronyms have been
  registered before reading the gdx data. We need to map an udefined index we read to a new value.
  The value of NextAutoAcronym is used for that.
*/
   int gdxAcronymGetMapping( int N, int &orgIndx, int &newIndx, int &autoIndex );
   // endregion

   // region Filter handling


   /**
@brief  Check if there is a filter defined based on its number

@param  FilterNr Filter number as used in FilterRegisterStart
@return
  Non-zero if the operation is possible, zero otherwise
@see gdxFilterRegisterStart
*/
   int gdxFilterExists( int FilterNr );


   /**
@brief  Define a unique element filter
@details
  Start the registration of a filter. A filter is used to map a number
  of elements to a single integer; the filter number. A filter number
  can later be used to specify a filter for an index postion when reading data.
@param  FilterNr Filter number to be assigned
@return
  Non-zero if the operation is possible, zero otherwise
@see gdxFilterRegister, gdxFilterRegisterDone, gdxDataReadFilteredStart

*/
   int gdxFilterRegisterStart( int FilterNr );
   /**
@brief  Add a unique element to the current filter definition
@details
 Register a unique element as part of the current filter. The
 function returns false if the index number is out of range of
 valid user indices or the index was never mapped into the
 user index space.
@param  UelMap Unique element number in the user index space
@return
  Non-zero if the operation is possible, zero otherwise
@see gdxFilterRegisterStart, gdxFilterRegisterDone

*/
   int gdxFilterRegister( int UelMap );


   /**
@brief  Finish registration of unique elements for a filter
@return
  Non-zero if the operation is possible, zero otherwise
@see gdxFilterRegisterStart, gdxFilterRegister
*/
   int gdxFilterRegisterDone();
   /**
@brief  Initialize the reading of a symbol in filtered mode

@param  SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
@param  FilterAction Array of filter actions for each index position
@param  NrRecs The maximum number of records available for reading. The actual number of records may be
     less when a filter is applied to the records read.
@return
  Non-zero if the operation is possible, zero otherwise
@see  gdxFilterRegisterStart, gdxDataReadMap, gdxDataReadRawStart, gdxDataReadStrStart, gdxDataReadDone
@details
  Start reading data for a symbol in filtered mode. Each filter action
  (1..Dimension) describes how each index should be treated when reading
  a data record. When new unique elements are returned, they are added
  to the user index space automatically. The actual reading of records
  is done with DataReadMap.
  <P>The action codes are as follows:

| Action code   | Result                                                         |
| ------------- | -------------------------------------------------------------- |
| DOMC_UNMAPPED | The index is not mapped into user space                         |
| DOMC_EXPAND   | New unique elements encountered will be mapped into the user space |
| DOMC_STRICT   | If the unique element in this position does not map into user space, the record will not be available and is added to the error list instead |
| FilterNumber  | If the unique element in this position does not map into user space or is not enabled in this filter, the record will not be available and is added to the error list instead |

*/
   int gdxDataReadFilteredStart( int SyNr, const int *FilterAction, int &NrRecs );
   // endregion

   /**
@brief  Set the Node number for an entry in the string table
@details
 After registering a string with AddSetText, we can assign
 a node number for later retrieval. The node number is any
 integer which is stored without further restrictions.
@param  TxtNr Index number of the entry to be modified
@param  Node The new Node value for the entry
@return
  Non-zero if the operation is possible, zero otherwise
@see gdxAddSetText, gdxGetElemText

*/
   int gdxSetTextNodeNr( int TxtNr, int Node );


   /**
@brief  Get the unique elements for a given dimension of a given symbol
@details
  Using the data of a symbol, get the unique elements for a given index position. To achieve this,
  the symbols data is read and a tally is kept for the elements in the given index position. When a filter
  is specified, records that have elements in the specified index position that are outside the filter will
  be added to the list of DataErrorRecords. See gdxDataErrorRecord
@param  SyNr The index number of the symbol, range 1..NrSymbols
@param  DimPos  The dimension to use, range 1..dim
@param  FilterNr Number of a previously registered filter or the value DOMC_EXPAND if no filter is wanted
@param  DP Callback procedure which will be called once for each available element (can be nil)
@param  NrElem Number of unique elements found
@param  UPtr User pointer; will be passed to the callback procedure
@return
  Non-zero if the operation is possible, zero otherwise

@see  gdxDataErrorCount gdxDataErrorRecord

@code
  var
     T0 : Cardinal;
     Cnt: integer;

  procedure DataDomainCB(RawNr, MappedNr: integer; UPtr: pointer); stdcall;
  begin
  Write(RawNr, ' (', MappedNr, ')');
  end;

  T0 := GetTickCount();
  gdxGetDomainElements(PGX, 1, 1, DOMC_EXPAND, nil, cnt);
  WriteLn('Domain count only = ',cnt ,' ', GetTickCount - T0, ' ms');
  T0 := GetTickCount();
  gdxGetDomainElements(PGX, 1, 1, DOMC_EXPAND, DataDomainCB, cnt);
  WriteLn('Get domain count = ',cnt ,' ', GetTickCount - T0, ' ms');
  T0 := GetTickCount();
  gdxGetDomainElements(PGX, 1, 1, 7, DataDomainCB, cnt);
  WriteLn('Using filter 7; number of records in error list = ', gdxDataErrorCount(PGX) );
@endcode
*/
   int gdxGetDomainElements( int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int &NrElem, void *UPtr );


   /**
@brief   Set the amount of trace (debug) information generated
 
@param   N Tracing level  N <= 0 no tracing  N >= 3 maximum tracing
@param   s A string to be included in the trace output
@return
  Always non-zero
*/
   int gdxSetTraceLevel( int N, const char *s );


   /**
@brief
  Add a new acronym entry
@details
  This function can be used to add entries before data is written. When entries
  are added implicitly use gdxAcronymSetInfo to update the table.
@param   AName Name of the acronym
@param   Txt Explanatory text of the acronym
@param   AIndx  Index value of the acronym
@return
  0 If the entry is not added because of a duplicate name using the same value fo the indx
  -1 If the entry is not added because of a duplicate name using a different value for the indx
  Otherwise the index into the acronym table (1..gdxAcronymCount)
@see   gdxAcronymGetInfo, gdxAcronymCount

*/
   int gdxAcronymAdd( const char *AName, const char *Txt, int AIndx );


   /**
@brief
  Get index value of an acronym
@param   V Input value possibly representing an acronym
@return
  Index of acronym value V; zero if V does not represent an acronym
@see    gdxAcronymValue
*/
   [[nodiscard]] int gdxAcronymIndex( double V ) const;


   /**
@brief
  Find the name of an acronym value
@param   V Input value possibly containing an acronym
@param   AName Name of acronym value or the empty string
@return
  Return non-zero if a name for the acronym is defined. Return
  zero if V does not represent an acronym value or a name
  is not defined. An unnamed acronym value will return a string
  of the form UnknownAcronymNNN; were NNN is the index of the acronym.
@see    gdxAcronymIndex
*/
   int gdxAcronymName( double V, char *AName );


   /**
@brief
  Create an acronym value based on the index
@param
  AIndx Index value; should be greater than zero
@return
  The calculated acronym value; zero if Indx is not positive
@see    gdxAcronymIndex
*/
   [[nodiscard]] double gdxAcronymValue( int AIndx ) const;


   /**
@brief
  Returns the value of the AutoConvert variable and sets the variable to nv
@details
  When we close a new gdx file, we look at the value of AutoConvert; if AutoConvert
  is non-zero, we look at the GDXCOMPRESS and GDXCONVERT environment variables to determine if
  conversion to an older file format is desired. We needed this logic so gdxcopy.exe
  can disable automatic file conversion.
@param
  nv New value for AutoConvert
@return
  Previous value of AutoConvert
 
*/
   int gdxAutoConvert( int nv );
   /**
 @brief
   Returns a version descriptor of the library
 @param
    V Contains version string after return
 @return
   Always returns non-zero*/
   static int gdxGetDLLVersion( char *V );


   /**
@brief
  Returns file format number and compression level used
 
@param  FileVer File format number or zero if the file is not open
@param  ComprLev Compression used; 0= no compression, 1=zlib
@return
  Always returns non-zero
 */
   int gdxFileInfo( int &FileVer, int &ComprLev ) const;


   /**
@brief   Prepare for the reading of a slice of data from a data set
@details
  Prepare for the reading of a slice of data. The actual read of the data
  is done by calling gdxDataReadSlice. When finished reading, call gdxDataReadDone.
@param  SyNr Symbol number to read, range 1..NrSymbols
@param   ElemCounts Array of integers, each position indicating the number of
             unique indices in that position
@return
  Non-zero if the operation is possible, zero otherwise


@see   gdxDataReadSlice, gdxDataReadDone

*/
   int gdxDataReadSliceStart( int SyNr, int *ElemCounts );


   /**
@brief   Read a slice of data from a data set
@details
  Read a slice of data, by fixing zero or more index positions in the data.
  When a data element is available, the callback procedure DP is called with the
  current index and the values. The indices used in the index vary from zero to
  the highest value minus one for that index position. This function can be called
  multiple times.
@param   UelFilterStr Each index can be fixed by setting the string for the unique
                  element. Set an index position to the empty string in order
                  not to fix that position.
@param   Dimen The dimension of the index space; this is the number of index positions
         that is not fixed.
@param   DP Callback procedure which will be called for each available data item
@return
  Non-zero if the operation is possible, zero otherwise


@see   gdxDataReadSliceStart, gdxDataSliceUELS, gdxDataReadDone

*/
   int gdxDataReadSlice( const char **UelFilterStr, int &Dimen, TDataStoreProc_t DP );


   /**
@brief   Map a slice index in to the corresponding unique elements
@details
  After calling DataReadSliceStart, each index position is mapped from 0 to N(d)-1.
  This function maps this index space back in to unique elements represented as
  strings.

@param  SliceKeyInt The slice index to be mapped to strings.
@param  KeyStr Array of strings containg the unique elements

@return
  Non-zero if the operation is possible, zero otherwise
@see   gdxDataReadSliceStart, gdxDataReadDone

*/
   int gdxDataSliceUELS( const int *SliceKeyInt, char **KeyStr );


   /**
@brief   Return the number of bytes used by the data objects
@return
  The number of bytes used by the data objects
*/
   int64_t gdxGetMemoryUsed();


   /**
@brief   Classify a value as a potential special value

@param  D Value to classify
@param  sv Classification
@return
  Returns non-zero if D is a special value, zero otherwise
@see  gdxGetSpecialValues, gdxSetSpecialValues
*/
   int gdxMapValue( double D, int &sv );


   /**
@brief   Open an existing gdx file for output
@details
  Open an existing gdx file for output. If a file extension is not
  supplied, the extension '.gdx' will be used. The return code is
  a system dependent I/O error.
  When appending to a gdx file, the symbol table, uel table etc will be read
  and the whole setup will be treated as if all symbols were just written to
  the gdx file. Replacing a symbol is not allowed; it will generate a duplicate
  symbol error.
@param  FileName  File name of the gdx file to be created
@param  Producer  Name of program that appends to the gdx file
@param  ErrNr Returns an error code or zero if there is no error
@return
  Returns non-zero if the file can be opened; zero otherwise
@see   gdxOpenRead, gdxOpenWrite, gdxOpenWriteEx


@code
  var
    ErrNr: integer;
    PGX  : PGXFile;
    Msg  : ShortString;
  begin
  if not gdxGetReady(Msg)
  then
    begin
    WriteLn('Cannot load GDX library, msg: ', Msg);
    exit;
    end;
  gdxOpenAppend(PGX,'c:\\mydata\\file1.gdx','Examples', ErrCode);
  if ErrCode <> 0
  then
    [ ... ]
@endcode
*/
   int gdxOpenAppend( const char *FileName, const char *Producer, int &ErrNr );


   /**
@brief   Test if any of the elements of the set has an associated text
@param
  SyNr Set Symbol number (1..NrSymbols)
@return
  Non-zero if the Set contains at least one element that has associated text,
    zero otherwise
@see   gdxSystemInfo, gdxSymbolInfo
*/
   int gdxSetHasText( int SyNr );


   /**
@brief   Set the internal values for special values when reading a gdx file
@param
  AVals array of special values to be used for Eps, +Inf, -Inf, NA and Undef
        Note that the values do not have to be unique
@return
  Always non-zero
@note Before calling this function, initialize the array of special values
  by calling gdxGetSpecialValues first
@see  gdxSetSpecialValues, gdxResetSpecialValues, gdxGetSpecialValues
*/
   int gdxSetReadSpecialValues( const double *AVals );


   /**
@brief
  Returns the length of the longest UEL used for every index position for a given symbol

@param  SyNr Symbol number
@param  LengthInfo The longest length for each index position
@return
  The length of the longest UEL found in the data
@see   gdxUELMaxLength
*/
   int gdxSymbIndxMaxLength( int SyNr, int *LengthInfo );


   /**
@brief
  Returns the length of the longest symbol name
@return
  The length of the longest symbol name
*/
   [[nodiscard]] int gdxSymbMaxLength() const;
   /**
@brief
  Add a line of comment text for a symbol

@param  SyNr The symbol number (range 1..NrSymbols); if SyNr <= 0 the current symbol being written
@param  Txt String to add
@return
  Non-zero if the operation is possible, zero otherwise
@see   gdxSymbolGetComment
*/
   int gdxSymbolAddComment( int SyNr, const char *Txt );
   /**
@brief
  Retrieve a line of comment text for a symbol

@param   SyNr The symbol number (range 1..NrSymbols)
@param   N Line number (1..Count)
@param   Txt String containing the line requested
@return
  Non-zero if the operation is possible, zero otherwise
@see   gdxSymbolAddComment
*/
   int gdxSymbolGetComment( int SyNr, int N, char *Txt );
   /**
@brief
  Returns the length of the longest UEL name
@return
  The length of the longest UEL name
@see   gdxSymbIndxMaxLength
*/
   [[nodiscard]] int gdxUELMaxLength() const;
   /**
@brief   Search for unique element by its string
@param   Uel String to be searched
@param   UelNr Internal unique element number or -1 if not found
@param   UelMap User mapping for the element or -1 if not found or
        the element was never mapped
@return
  Non-zero if the element was found, zero otherwise
*/
   int gdxUMFindUEL( const char *Uel, int &UelNr, int &UelMap );

   [[nodiscard]] int gdxStoreDomainSets() const;
   void gdxStoreDomainSetsSet( int x );

   /**
@brief   Read a symbol in Raw mode while applying a filter using a callback procedure
@details
  Read a slice of data, by fixing zero or more index positions in the data.
  When a data element is available, the callback procedure DP is called with the
  current index (as raw numbers) and the values.


@param  UelFilterStr Each index can be fixed by setting the string for the unique
                element. Set an index position to the empty string in order
                not to fix that position.
@param  DP Callback procedure which will be called for each available data item

@return
  Non-zero if the operation is possible, zero otherwise

@see   gdxDataReadRawFast, gdxDataReadSliceStart, gdxDataSliceUELS, gdxDataReadDone,


@code
function DPCallBack(const Indx: TgdxUELIndex; const Vals: TgdxValues; Uptr: Pointer): integer; stdcall;
var
  s: ShortString;
  UelMap: integer;
begin
Result := 1;
gdxUMUelGet(Uptr, Indx[2], s, UelMap);
WriteLn(s, ' ', Vals[vallevel]);
end;

var
  pgx  : PGXFile;
  Msg  : ShortString;
  ErrNr: integer;
  IndxS: TgdxStrIndex;

IndxS[1] := 'i200'; IndxS[2] := '';
gdxDataReadRawFastFilt(pgx, 1, IndxS, DPCallBack);
@endcode
*/
   int gdxDataReadRawFastFilt( int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP );
   /**
@brief   Read a symbol in Raw mode using a callback procedure
@details
  Use a callback function to read a symbol in raw mode. Using a callback procedure
  to read the data is faster because we no longer have to check the context for each
  call to read a record.
@param  SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
@param  DP Procedure that will be called for each data record
@param  NrRecs The number of records available for reading
@return
  Non-zero if the operation is possible, zero otherwise
@see   gdxDataReadRaw, gdxDataReadMapStart, gdxDataReadStrStart, gdxDataReadDone, gdxDataReadRawFastFilt

*/
   int gdxDataReadRawFast( int SyNr, TDataStoreProc_t DP, int &NrRecs );
   /**
@brief   Read a symbol in Raw mode using a callback procedure
@details
  Use a callback function to read a symbol in raw mode. Using a callback procedure
  to read the data is faster because we no longer have to check the context for each
  call to read a record.
@param  SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe
@param  DP Procedure that will be called for each data record
@param  NrRecs The number of records available for reading
@param  Uptr pointer to user memory that will be passed back with the callback
@return
  Non-zero if the operation is possible, zero otherwise
@see   gdxDataReadRawFast

*/
   int gdxDataReadRawFastEx( int SyNr, TDataStoreExProc_t DP, int &NrRecs, void *Uptr );

private:
   std::unique_ptr<gmsstrm::TMiBufferedStreamDelphi> FFile;
   TgxFileMode fmode{ f_not_open }, fmode_AftReg{ f_not_open };
   enum
   {
      stat_notopen,
      stat_read,
      stat_write
   } fstatus{ stat_notopen };
   int fComprLev{};
   std::unique_ptr<TUELTable> UELTable;
   std::unique_ptr<TSetTextList> SetTextList{};
   std::unique_ptr<int[]> MapSetText{};
   int FCurrentDim{};
   std::array<int, GLOBAL_MAX_INDEX_DIM> LastElem{}, PrevElem{}, MinElem{}, MaxElem{};
   std::array<std::array<char, GLOBAL_UEL_IDENT_SIZE>, GLOBAL_MAX_INDEX_DIM> LastStrElem{};
   int DataSize{};
   tvarvaltype LastDataField{};
   std::unique_ptr<TNameList> NameList;
   std::unique_ptr<TDomainStrList> DomainStrList;
   std::unique_ptr<LinkedDataType> SortList;
   std::optional<LinkedDataIteratorType> ReadPtr;
   std::unique_ptr<TTblGamsDataImpl<double>> ErrorList;
   PgdxSymbRecord CurSyPtr{};
   int ErrCnt{}, ErrCntTotal{};
   int LastError{}, LastRepError{};
   std::unique_ptr<TFilterList> FilterList;
   TDFilter *CurFilter{};
   TDomainList DomainList{};
   bool StoreDomainSets{ true };
   TIntlValueMapDbl intlValueMapDbl{}, readIntlValueMapDbl{};
   TIntlValueMapI64 intlValueMapI64{};
   TraceLevels TraceLevel{ TraceLevels::trl_all };
   std::string TraceStr;
   int VersionRead{};
   std::string FProducer, FProducer2, FileSystemID;
   int64_t MajorIndexPosition{};
   int64_t NextWritePosition{};
   int DataCount{}, NrMappedAdded{};
   std::array<TgdxElemSize, GLOBAL_MAX_INDEX_DIM> ElemType{};
   std::string MajContext;
   std::array<std::optional<TIntegerMapping>, GLOBAL_MAX_INDEX_DIM> SliceIndxs, SliceRevMap;
   int SliceSyNr{};
   std::array<std::string, GMS_MAX_INDEX_DIM> SliceElems;
   bool DoUncompress{},  // when reading
           CompressOut{};// when writing
   int DeltaForWrite{};  // delta for last dimension or first changed dimension
   int DeltaForRead{};   // first position indicating change
   double Zvalacr{};     // tricky
   std::unique_ptr<TAcronymList> AcronymList;
   std::array<TSetBitMap *, GLOBAL_MAX_INDEX_DIM> WrBitMaps{};
   bool ReadUniverse{};
   int UniverseNr{}, UelCntOrig{};// original uel count when we open the file
   int AutoConvert{ 1 };
   int NextAutoAcronym{};
   bool AppendActive{};

#ifndef VERBOSE_TRACE
   const TraceLevels defaultTraceLevel{ TraceLevels::trl_none };
   const bool verboseTrace{};
#else
   const TraceLevels defaultTraceLevel{ TraceLevels::trl_all };
   const bool verboseTrace{ true };
#endif

   //api wrapper magic for Fortran
   TDataStoreFiltProc_t gdxDataReadRawFastFilt_DP{};
   TDomainIndexProc_t gdxGetDomainElements_DP{};

   bool PrepareSymbolWrite( std::string_view Caller, const char *AName, const char *AText, int ADim, int AType, int AUserInfo );
   int PrepareSymbolRead( std::string_view Caller, int SyNr, const int *ADomainNrs, TgxFileMode newmode );

   void InitErrors();
   void SetError( int N );
   void ReportError( int N );
   bool ErrorCondition( bool cnd, int N );

   bool MajorCheckMode( std::string_view Routine, TgxFileMode m );
   bool MajorCheckMode( std::string_view Routine, const TgxModeSet &MS );

   bool CheckMode( std::string_view Routine );
   bool CheckMode( std::string_view Routine, TgxFileMode m );
   bool CheckMode( std::string_view Routine, const TgxModeSet &MS );

   void WriteTrace( std::string_view s ) const;
   void InitDoWrite( int NrRecs );
   bool DoWrite( const int *AElements, const double *AVals );
   bool DoRead( double *AVals, int &AFDim );
   void AddToErrorListDomErrs( const std::array<int, GLOBAL_MAX_INDEX_DIM> &AElements, const double *AVals );
   void AddToErrorList( const int *AElements, const double *AVals );
   void GetDefaultRecord( double *Avals ) const;
   double AcronymRemap( double V );
   bool IsGoodNewSymbol( const char *s );
   bool ResultWillBeSorted( const int *ADomainNrs );

   int gdxOpenReadXX( const char *Afn, int filemode, int ReadMode, int &ErrNr );

   // This one is a helper function for a callback from a Fortran client
   void gdxGetDomainElements_DP_FC( int RawIndex, int MappedIndex, void *Uptr );
   int gdxDataReadRawFastFilt_DP_FC( const int *Indx, const double *Vals, void *Uptr );

public:
   bool gdxGetDomainElements_DP_CallByRef{},
           gdxDataReadRawFastFilt_DP_CallByRef{},
           gdxDataReadRawFastEx_DP_CallByRef{};
};

}// namespace gdx
