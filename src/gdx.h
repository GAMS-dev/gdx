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

/// @details Class for reading and writing GDX files through a efficient low-level interface
class TGXFileObj
{
public:
   /**
    * Constructor of GDX file object
    * @brief Create a new GDX file object. Does not open a file yet.
    * @param ErrMsg Out argument for storing potential error messages. Will be empty when there is no error.
     */
   explicit TGXFileObj( std::string &ErrMsg );

   /**
    * Destructor of GDX file object
    * @brief Dispose GDX file object.
    * @details Tears down a file GDX object and potentially closes (and flushes) a file opened for writing.
    */
   ~TGXFileObj();

   /**
    * @brief Open a new GDX file for output. Non-zero if the file can be opened, zero otherwise.
    * @details
    *   Uses the environment variable GDXCOMPRESS to set compression argument for gdxOpenWriteEx.
    *   Potentially overwrites existing file with same name.
    *   If a file extension is not supplied, the extension '.gdx' will be used. The return code is
    *   a system dependent I/O error.
    * @param FileName File name of the GDX file to be created with arbitrary length.
    * @param Producer Name of program that creates the GDX file (should not exceed 255 characters).
    * @param ErrNr Returns an error code or zero if there is no error.
    * @return Returns non-zero if the file can be opened; zero otherwise.
    * @see gdxOpenRead, gdxOpenWriteEx
    */
   int gdxOpenWrite( const char *FileName, const char *Producer, int &ErrNr );

   /**
    * @brief Create a GDX file for writing with explicitly given compression flag. Non-zero if the file can be opened, zero otherwise.
    * @details Open a new GDX file for output. If a file extension is not
    * supplied, the extension '.gdx' will be used. The return code is
    * a system dependent I/O error.
    * @param FileName File name of the GDX file to be created with arbitrary length.
    * @param Producer Name of program that creates the GDX file (should not exceed 255 characters).
    * @param Compr Zero for no compression; non-zero uses compression (if available).
    * @param ErrNr Returns an error code or zero if there is no error.
    * @attention When writing compressed, set the AutoConvert flag to zero so the file is not uncompressed after the gdxClose; see gdxAutoConvert.
    * @return Returns non-zero if the file can be opened; zero otherwise.
    * @code
      std::string errMsg;
      TGXFileObj pgx{errMsg};
      if(!errMsg.empty()) {
          std::cout << "Failure with GDX: " << errMsg << std::endl;
          return;
      }
      int ErrCode;
      pgx.gdxOpenWriteEx("file1.gdx", "Examples", 1, ErrCode);
      pgx.gdxAutoConvert(0);
      if(ErrCode) {
      [ ... ]
      }
    * @endcode
    * @see gdxOpenRead, gdxOpenWrite, gdxAutoConvert
    */
   int gdxOpenWriteEx( const char *FileName, const char *Producer, int Compr, int &ErrNr );

   /**
    * @brief Start writing a new symbol in string mode. Returns zero if the operation is not possible or failed.
    * @details Adds a new symbol and supplies the UEL keys of the records for each dimension as strings.
    *   UEL labels can be known or new (in which case they are added to the UEL table).
    * @param SyId Name of the symbol (limited to 63 characters).
    *   The first character of a symbol must be a letter.
    *   Following symbol characters may be letters, digits, and underscores.
    *   Symbol names must be new and unique.
    * @param ExplTxt Explanatory text for the symbol (limited to 255 characters). Mixed quote characters will be unified to first occurring one.
    * @param Dimen Dimension of the symbol (limited to 20).
    * @param Typ Type of the symbol (set=0, parameter=1, variable=2, equation=3, alias=4).
    * @param UserInfo Supply additional data. See gdxDataWriteRawStart for more information.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataWriteStr, gdxDataWriteDone
    */
   int gdxDataWriteStrStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );

   /**
    * @brief Write a data element in string mode. Each element string must follow the GAMS rules for unique elements. Returns zero if the operation is not possible.
    * @details
    *   <ul><li>When writing data using string elements, each string element is added
    *   to the internal unique element (UEL) table and assigned an index.</li>
    *   <li>Writing using strings does not add the unique elements to the
    *   user mapped space.</li>
    *   <li>Each element string must follow the GAMS rules for unique elements
    *   e.g. not exceeding 63 characters in length and not mixing single-
    *   and double-quotes.</li></ul>
    * @attention
    *   This write operation affects the in-memory GDX object.
    *   Actual flushing of the records to the GDX file happens in gdxDataWriteDone.
    *   KeyStr should point to one string for each symbol dimension.
    *   Each key string should not be longer than 63 characters.
    *   Values should be big enough to store 5 double values.
    *   Make sure there is a key string for each symbol dimension and each key string does not exceed 63 characters.
    *   Make sure values does not contain more than 5 entries.
    * @param KeyStr The index for this element using strings for the unique elements. One entry for each symbol dimension.
    * @param Values The values for this element (level, marginal, lower-, upper-bound, scale).
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataWriteMapStart, gdxDataWriteDone
    */
   int gdxDataWriteStr( const char **KeyStr, const double *Values );

   /**
    * @brief Finish a write operation. Returns zero if the operation is not possible.
    * @details For mapped- and string-mode the actual writing of the records to the GDX file happens here.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataErrorCount, gdxDataWriteRawStart, gdxDataWriteMapStart, gdxDataWriteStrStart
    */
   int gdxDataWriteDone();

   /**
    * @brief Start registering unique elements in mapped mode. Returns zero if the operation is not possible.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxUELRegisterMap, gdxUELRegisterDone
    */
   int gdxUELRegisterMapStart();

   /**
    * @brief Register unique element in mapped mode. A unique element must follow the GAMS rules when it contains quote characters. Returns zero if the operation is not possible.
    * @details
    *  UMap is the user assigned index for the element.
    *  Registering an element a second time is not considered an error as long as the same UMap is used.
    *  Assigning different elements with the same UMap value is an error.
    *  A unique element must follow the GAMS rules when it contains quote characters
    *  and not exceed 63 characters length.
    * @param UMap User index number to be assigned to the unique element, -1 if not found or the element was never mapped.
    * @param Uel String for unique element (max. 63 chars and no single-/double-quote mixing).
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxUELRegisterMapStart, gdxUELRegisterDone
    */
   int gdxUELRegisterMap( int UMap, const char *Uel );

   /**
    * @brief Close a GDX file that was previously opened for reading or writing.
    *   Before the file is closed, any pending write operations will be
    *   finished. This does not free the GDX in-memory object.
    *   This method will automatically be called when the GDX object
    *   lifetime ends (e.g. being out of scope).
    * @return Returns the value of gdxGetLastError.
    * @see gdxOpenRead, gdxOpenWrite
    */
   int gdxClose();

   /**
    * @brief Reset the internal values for special values. Always non-zero.
    * @return Always non-zero.
    * @see gdxSetSpecialValues, gdxGetSpecialValues
    */
   int gdxResetSpecialValues();

   /**
    * @brief Returns the text for a given error number. Always non-zero.
    * @param ErrNr Error number.
    * @param ErrMsg Error message (output argument). Contains error text after return.
    * @return Always returns non-zero.
    * @attention Supplied buffer for error message ErrMsg should be at least 256 bytes long.
    * @see gdxGetLastError
    */
   static int gdxErrorStr( int ErrNr, char *ErrMsg );

   /**
    * @brief Open a GDX file for reading. Non-zero if the file can be opened, zero otherwise.
    * @details
    *    Open an existing GDX file for input. If a file extension is not
    *    supplied, the extension '.gdx' will be used. The return code is
    *    a system dependent I/O error. If the file was found, but is not
    *    a valid GDX file, the function GetLastError can be used to handle
    *    these type of errors.
    * @param FileName File name of the GDX file to be opened (arbitrary length).
    * @param ErrNr Returns an error code or zero if there is no error.
    * @return Returns non-zero if the file can be opened; zero otherwise.
    * @see gdxOpenWrite, gdxGetLastError
    * @code
      std::string errMsg;
      TGXFileObj pgx{errMsg};
      pgx.gdxOpenRead("file1.gdx", ErrNr);
      if(ErrNr) {
        [...]
      }
    * @endcode
    */
   int gdxOpenRead( const char *FileName, int &ErrNr );

   /**
    * @brief Return strings for file version and file producer. Always non-zero.
    * @param FileStr Version string (out argument). Known versions are V5, V6U, V6C and V7.
    * @param ProduceStr Producer string (out argument). The producer is the application that wrote the GDX file.
    * @return Always non-zero.
    * @attention Supplied buffers for FileStr and ProduceStr should be 256 bytes long to prevent overflow.
    * @see gdxOpenWrite, gdxOpenWriteEx
    */
   int gdxFileVersion( char *FileStr, char *ProduceStr ) const;

   /**
    * @brief Search for a symbol by name in the symbol table; the search is not case-sensitive.
    *    <ul><li>When the symbol is found, SyNr contains the symbol number and the
    *    function returns a non-zero integer.</li>
    *    <li>When the symbol is not found, the function returns zero and SyNr is set to -1.</li></ul>
    * @param SyId Name of the symbol (must not exceed 63 characters).
    * @param SyNr Symbol number (>=1 if exists, 0 for universe and -1 if not found).
    * @return Non-zero if the symbol is found, zero otherwise.
    * @see gdxSymbolInfo, gdxSymbolInfoX
    */
   int gdxFindSymbol( const char *SyId, int &SyNr );

   /**
    * @brief Read the next record using strings for the unique elements. The reading should be initialized by calling DataReadStrStart. Returns zero if the operation is not possible or if there is no more data.
    * @param KeyStr The index of the record as strings for the unique elements. Array of strings with one string for each dimension.
    * @param Values The data of the record (level, marginal, lower-, upper-bound, scale).
    * @param DimFrst The first index position in KeyStr that changed.
    * @return
    *    Non-zero if the operation is possible; return zero if the operation is not
    *    possible or if there is no more data.
    * @attention
    *   KeyStr must point to one string for each symbol dimension where each string buffer must have size of 64 bytes.
    *   Values must have length >=5 double entries.
    * @see gdxDataReadStrStart, gdxDataReadDone
    */
   int gdxDataReadStr( char **KeyStr, double *Values, int &DimFrst );

   /**
    * @brief Finish reading of a symbol in any mode (raw, mapped, string). . Returns zero if the operation is not possible.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadRawStart, gdxDataReadMapStart, gdxDataReadStrStart
    */
   int gdxDataReadDone();

   /**
    * @brief Returns information (name, dimension count, type) about a symbol from the symbol table. Returns zero if the symbol number is out of range, non-zero otherwise.
    * @param SyNr The symbol number (range 0..NrSymbols); return universe info (*) when SyNr = 0.
    * @param SyId Name of the symbol (buffer should be 64 bytes long). Magic name "*" for universe.
    * @param Dimen Dimension of the symbol (range 0..20).
    * @param Typ Symbol type (set=0, parameter=1, variable=2, equation=3, alias=4).
    * @return Zero if the symbol number is not in the correct range, non-zero otherwise.
    * @attention SyId must be 64 characters long.
    * @see gdxSystemInfo, gdxSymbolInfoX, gdxSymbolDim, gdxFindSymbol
    */
   int gdxSymbolInfo( int SyNr, char *SyId, int &Dimen, int &Typ );

   /**
    * @brief Initialize the reading of a symbol in string mode. Returns zero if the operation is not possible.
    * @details
    *   Reading data using strings is the simplest way to read data.
    *   Every record read using DataReadStr will return the strings
    *   for the unique elements. Internal mapping is not affected by
    *   this function.
    * @param SyNr The index number of the symbol (range 0..NrSymbols); SyNr = 0 reads universe.
    * @param NrRecs The maximum number of records available for reading. The actual number of records may be less when a filter is applied to the records read.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadStr, gdxDataReadRawStart, gdxDataReadMapStart, gdxDataReadDone
    * @code
      if(pgx.gdxDataReadStrStart(1,NrRecs)) {
        while(pgx.gdxDataReadStr(Uels,Vals)) {
          ...
        }
        pgx.gdxDataReadDone();
      }
    * @endcode
    */
   int gdxDataReadStrStart( int SyNr, int &NrRecs );

   /**
    * @brief Add an alias for a set to the symbol table.
    *    One of the two identifiers has to be a known set, an alias or "*" (universe);
    *    the other identifier is used as the new alias for the given set.
    *    The function gdxSymbolInfoX can be used to retrieve the set or alias
    *       associated with the identifier; it is returned as the UserInfo parameter.
    * @param Id1 First set identifier.
    * @param Id2 Second set identifier.
    * @attention One of the set identifiers must be a novel unique name.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxSymbolSetDomain
    */
   int gdxAddAlias( const char *Id1, const char *Id2 );


   /**
    * @brief Register a string in the string table
    *   Register a string in the string table and return the integer number assigned to this string.
    *   The integer value can be used to set the associated text of a set element.
    *   The string must follow the GAMS syntax rules for explanatory text
    *   e.g. not longer than 255 characters.
    * @attention Mixing of single- and double-quotes in the explanatory text will be resolved by replacing
    *   all quote character occurrences with the first one in the text.
    * @param Txt The string to be registered (must not exceed 255 characters).
    * @param TxtNr The index number assigned to this string (output argument).
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxGetElemText, gdxSetTextNodeNr
    */
   int gdxAddSetText( const char *Txt, int &TxtNr );


   /**
    * @brief Query the number of error records.
    * @details
    *   <p>After a write operation is finished (with gdxDataWriteDone), the data
    *   is sorted and written to the GDX file (for map- and string-mode).
    *   If there are duplicate records, the first record is written to the file and
    *   the duplicates are added to the error list.</p>
    *   <p>When reading data using a filtered read operation, data records that were
    *   filtered out because an index is not in the user index space or not in a
    *   filter are added the error list.</p>
    * @return The number of error records available.
    * @see gdxDataErrorRecord
    */
   [[nodiscard]] int gdxDataErrorCount() const;

   /**
    * @brief Retrieve an error record. Non-zero if the record number is valid.
    * @details Does not indicate domain violation for filtered/strict read with negative indices.
    * @param RecNr The number of the record to be retrieved (range = 1..NrErrorRecords); this argument is ignored in gdxDataReadMap
    * @param KeyInt Index for the record (array of UEL numbers for each dimension).
    * @param Values Values for the record (level, marginal, lower-, upper-bound, scale).
    * @return Non-zero if the record number is valid, zero otherwise.
    * @attention
    *   Same as gdxDataErrorRecordX but negative UEL index numbers (for domain violations) are inverted,
    *   so the index is always >=0.
    *   KeyInt must be big enough to hold one UEL index for each dimension!
    *   Values must have length >=5.
    * @see gdxDataErrorCount
    */
   int gdxDataErrorRecord( int RecNr, int *KeyInt, double *Values );

   /**
    * @brief Retrieve an error record. Non-zero if the record number is valid.
    * @details Also indicate domain violations for filtered/strict read with negative UEL index values.
    * @param RecNr The number of the record to be retrieved, (range 1..NrErrorRecords); this argument is ignored in gdxDataReadMap
    * @param KeyInt Index for the record, negative uel indicates domain violation for filtered/strict read.
    * @param Values Values for the record (level, marginal, lower-, upper-bound, scale).
    * @return Non-zero if the record number is valid, zero otherwise.
    * @attention
    *   KeyInt must be big enough to hold one UEL index for each dimension!
    *   Values must have length >=5.
    * @see gdxDataErrorCount
    */
   int gdxDataErrorRecordX( int RecNr, int *KeyInt, double *Values );

   /**
    * @brief Read the next record in raw mode. Returns zero if the operation is not possible.
    * @param KeyInt The index of the record in UEL numbers for each dimension.
    * @param Values The data of the record (level, marginal, lower-, upper-bound, scale).
    * @param DimFrst The first index position in KeyInt that changed.
    * @return Non-zero if the operation is possible, zero otherwise (e.g. no records left).
    * @attention
    *   KeyInt must be big enough to hold one UEL index for each dimension!
    *   Values must have length >=5.
    * @see gdxDataReadRawStart, gdxDataReadDone
    */
   int gdxDataReadRaw( int *KeyInt, double *Values, int &DimFrst );

   /**
    * @brief Initialize the reading of a symbol in raw mode. Returns zero if the operation is not possible.
    * @param SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe.
    * @param NrRecs The maximum number of records available for reading. The actual number of records may be less when a filter is applied to the records read.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadRaw, gdxDataReadMapStart, gdxDataReadStrStart, gdxDataReadDone
    */
   int gdxDataReadRawStart( int SyNr, int &NrRecs );

   /**
    * @brief Write a data element in raw mode. Returns zero if the operation is not possible.
    * @details
    *   <p>When writing data in raw mode, the index space used is based on the
    *   internal index space. The indices used are in the range 1..NrUels but this is not enforced.
    *   Before we can write in raw mode, the unique elements (strings) should
    *   be registered first.</p>
    *   <p>When writing raw, it assumed that the records are written in sorted order and
    *   that there are no duplicate records. Records that are not in sorted order or are
    *   duplicates will be added to the error list (see DataErrorCount and DataErrorRecord).</p>
    * @param KeyInt The index for this element.
    * @param Values The values for this element.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataWriteRawStart, gdxDataWriteDone
    */
   int gdxDataWriteRaw( const int *KeyInt, const double *Values );

   /**
    * @brief Start writing a new symbol in raw mode. Returns zero if the operation is not possible.
    * @details
    *   Raw mode flushes new records immediately to the GDX file (unlike mapped or string mode).
    *   The key indices for the record are provided as unique element numbers.
    * @param SyId Name of the symbol (up to 63 characters).
    *   The first character of a symbol must be a letter.
    *   Following symbol characters may be letters, digits, and underscores.
    *   Symbol names must be new and unique.
    * @param ExplTxt Explanatory text for the symbol (up to 255 characters).
    * @param Dimen Dimension of the symbol (up to 20).
    * @param Typ Type of the symbol (set=0, parameter=1, variable=2, equation=3, alias=4).
    * @param UserInfo User field value storing additional data; GAMS follows the following conventions:
    *  <table>
    *  <thead>
    *  <tr>
    *    <th>Type</th>
    *    <th>Value(s)</th>
    *  </tr>
    *  </thead>
    *  <tbody>
    *  <tr>
    *    <td>Aliased Set</td>
    *    <td>The symbol number of the aliased set, or zero for the universe</td>
    *  </tr>
    *  <tr>
    *    <td>Set</td>
    *    <td>Zero</td>
    *  </tr>
    *  <tr>
    *    <td>Parameter</td>
    *    <td>Zero</td>
    *  </tr>
    *  <tr>
    *    <td>Variable</td>
    *    <td>The variable type: binary=1, integer=2, positive=3, negative=4, free=5, sos1=6, sos2=7, semicontinous=8,
    *        semiinteger=9
    *    </td>
    *  </tr>
    *  <tr>
    *    <td>Equation</td>
    *    <td>The equation type: eque=53, equg=54, equl=55, equn=56, equx=57, equc=58, equb=59</td>
    *  </tr>
    *  </tbody>
    *  </table>
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataWriteRaw, gdxDataWriteDone
    */
   int gdxDataWriteRawStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );

   /**
    * @brief Returns the number of errors.
    * @return Total number of errors encountered.
    * @see gdxGetLastError
    */
   [[nodiscard]] int gdxErrorCount() const;


   /**
    * @brief Retrieve the string and node number for an entry in the string table. Returns zero if the operation is not possible.
    * @details
    *   <ul><li>When writing to a GDX file, this index is the value returned by calling gdxAddSetText.</li>
    *   <li>When reading a GDX file, the index is returned as the level value when reading a set.</li></ul>
    *   The Node number can be used as an index in a string table in the user space;
    *   the value is set by calling SetTextNodeNr. If the Node number was never assigned,
    *   it will be returned as zero.
    * @param TxtNr String table index.
    * @param Txt Text found for the entry. Buffer should be 256 bytes wide.
    * @param Node Node number (user space) found for the entry.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxAddSetText, gdxSetTextNodeNr
    * @attention Buffer supplied in out argument string Txt should be 256 bytes wide to prevent overflow!
    * @code
      // assumes we are reading using strings
      while(pgx.gdxDataReadStr(Uels, Vals)) {
         for(int D{}; D<Dim; D++)
            std::cout << Uels[D] << " ";
         int indx = std::round(Vals[vallevel]);
         if(indx > 0) {
            pgx.gdxGetElemText(indx, S, N);
            std::cout << "txt = " << S << "Node = " << N;
         }
         std::cout << std::endl;
      }
    * @endcode
    */
   int gdxGetElemText( int TxtNr, char *Txt, int &Node );

   /**
    * @brief Returns the last error number or zero if there was no error. Calling this function will clear the last error stored.
    * @details
       When an error is encountered, an error code is stored which can
       be retrieved with this function. If subsequent errors occur before
       this function is called, the first error code will be maintained.
       Calling this function will clear the last error stored.
    * @return The error number, or zero if there was no error.
    * @see gdxErrorCount
    */
   int gdxGetLastError();


   /**
    * @brief Retrieve the internal values for special values. Always non-zero.
    * @param AVals 7-element array of special values used for Eps, +Inf, -Inf, NA and Undef.
    * @return Always non-zero.
    * @attention Output argument array Avals should have size for 7 elements.
    * @see gdxResetSpecialValues, gdxSetSpecialValues
    */
   int gdxGetSpecialValues( double *AVals );


   /**
    * @brief Set the internal values for special values. Before calling this function, initialize the array of special values by calling gdxGetSpecialValues first. Note, values in AVals have to be unique. Non-zero if all values specified are unique, zero otherwise.
    * @param AVals Array of special values to be used for Eps, +Inf, -Inf, NA and Undef.
    *   Note that the values have to be unique and AVals should have length 7.
    * @return Non-zero if all values specified are unique, zero otherwise.
    * @note Before calling this function, initialize the array of special values
    *   by calling gdxGetSpecialValues first.
    * @attention Size of AVals should be 7.
    * @see gdxSetReadSpecialValues, gdxResetSpecialValues, gdxGetSpecialValues
    */
   int gdxSetSpecialValues( const double *AVals );

   /**
    * @brief Retrieve the domain of a symbol. Returns zero if the operation is not possible.
    * @param SyNr The index number of the symbol (range 1..NrSymbols); SyNr = 0 reads universe.
    * @param DomainSyNrs Array (length=symbol dim) returning the set identifiers or "*";
    *   DomainSyNrs[D] will contain the index number of the one dimensional
    *   set or alias used as the domain for index position D. A value of zero represents the universe "*".
    * @return Non-zero if the operation is possible, zero otherwise.
    * @attention Integer array DomainSyNrs should be able to store one entry for each symbol dimension.
    * @see gdxSymbolSetDomain, gdxSymbolGetDomainX
    */
   int gdxSymbolGetDomain( int SyNr, int *DomainSyNrs );

   /**
    * @brief Retrieve the domain of a symbol (using relaxed or domain information). Returns zero if the operation is not possible.
    * @param SyNr The index number of the symbol (range 1..NrSymbols); SyNr = 0 reads universe.
    * @param DomainIDs DomainIDs[D] will contain the strings as they were stored with the call gdxSymbolSetDomainX.
    *   If gdxSymbolSetDomainX was never called, but gdxSymbolSetDomain was called, that information will be used instead.
    *   Length of this array should by dimensionality of the symbol.
    *   The special domain name "*" denotes the universe domain (all known UELs).
    * @return
    *   <ul><li>0: If operation was not possible (Bad SyNr)</li>
    *   <li>1: No domain information was available</li>
    *   <li>2: Data used was defined using gdxSymbolSetDomainX</li>
    *   <li>3: Data used was defined using gdxSymbolSetDomain</li></ul>
    * @attention
    *   Make sure DomainIDs is able to store a domain name for each symbol dimension.
    *   Each domain name requires a 256 byte wide buffer to store its maximum of 255 characters.
    * @see gdxSymbolSetDomainX, gdxSymbolSetDomain
    */
   int gdxSymbolGetDomainX( int SyNr, char **DomainIDs );

   /**
    * @brief Returns dimensionality of a symbol.
    * @param SyNr The symbol number (range 0..NrSymbols); return universe info when SyNr = 0..
    * @return -1 if the symbol number is not in the correct range, the symbol dimension otherwise.
    * @see gdxSymbolInfo, gdxSymbolInfoX, gdxFindSymbol
    */
   int gdxSymbolDim( int SyNr );

   /**
    * @brief Returns additional information about a symbol. Returns zero if the symbol number is out of range, non-zero otherwise.
    * @param SyNr The symbol number (range 0..NrSymbols); return universe info when SyNr = 0.
    * @param RecCnt Total number of records stored (unmapped); for the universe (SyNr = 0) this is the
    *      number of entries when the GDX file was opened for reading.
    * @param UserInfo User field value storing additional data; GAMS follows the following conventions:
    * <table>
    *  <thead>
    *  <tr>
    *      <th>Type</th>
    *      <th>Value(s)</th>
    *  </tr>
    *  </thead>
    *  <tbody>
    *  <tr>
    *      <td>Aliased Set</td>
    *      <td>The symbol number of the aliased set, or zero for the universe</td>
    *  </tr>
    *  <tr>
    *      <td>Set</td>
    *      <td>Zero</td>
    *  </tr>
    *  <tr>
    *      <td>Parameter</td>
    *      <td>Zero</td>
    *  </tr>
    *  <tr>
    *      <td>Variable</td>
    *      <td>The variable type: binary=1, integer=2, positive=3, negative=4, free=5, sos1=6, sos2=7, semicontinous=8,
    *          semiinteger=9
    *      </td>
    *  </tr>
    *  <tr>
    *      <td>Equation</td>
    *      <td>The equation type: eque=53, equg=54, equl=55, equn=56, equx=57, equc=58, equb=59</td>
    *  </tr>
    *  </tbody>
    *  </table>
    * @param ExplTxt Explanatory text for the symbol. Buffer for this output argument should be 256 bytes long.
    * @return Zero if the symbol number is not in the correct range, non-zero otherwise.
    * @attention Make sure the output argument string buffer for ExplTxt has size of 256 bytes.
    * @see gdxSystemInfo, gdxSymbolInfo, gdxFindSymbol
    */
   int gdxSymbolInfoX( int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt );

   /**
    * @brief Define the domain of a symbol for which a write data operation just started using DataWriteRawStart, DataWriteMapStart or DataWriteStrStart. Returns zero if the operation is not possible.
    * @details
    *   <p>This function defines the domain for the symbol for which a write
    *   data operation just started using DataWriteRawStart, DataWriteMapStart or
    *   DataWriteStrStart. At this point the symbol and dimension is known,
    *   but no data has been written yet.</p>
    *   <p>Each identifier will be checked to be a one dimensional set or an alias.
    *   When a domain is specified, write operations will be domain checked; records
    *   violating the domain will be added the the internal error list (see DataErrorCount
    *   and DataErrorRecord).</p>
    * @param DomainIDs Array of identifers (domain names) or "*" (universe). One domain name for each symbol dimension.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @attention Make sure there is one buffer with size 64 bytes for each symbol dimension.
    * @see gdxSymbolGetDomain
    */
   int gdxSymbolSetDomain( const char **DomainIDs );

   /**
    * @brief Define the domain of a symbol (relaxed version). Returns zero if the operation is not possible.
    * @details
    *   This function defines the relaxed domain information for the symbol SyNr.
    *   The identifiers will NOT be checked to be known one-dimensional sets, and
    *   no domain checking will be performed. This function can be called during or after
    *   the write operation.
    *   If domain checking is needed, use gdxSymbolSetDomain.
    * @param SyNr The index number of the symbol, range from 0 to NrSymbols; SyNr = 0 reads universe.
    * @param DomainIDs Array of identifiers (domain names) or "*" (universe). One domain name per symbol dimension with not more than 63 characters.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxSymbolSetDomain, gdxSymbolGetDomainX
    */
   int gdxSymbolSetDomainX( int SyNr, const char **DomainIDs );

   /**
    * @brief Returns the number of symbols and unique elements. Always non-zero.
    * @param SyCnt Number of symbols (sets, parameters, ...) available in the GDX file.
    * @param UelCnt Number of unique elements (UELs) stored in the GDX file.
    * @return Returns a non-zero value.
    */
   int gdxSystemInfo( int &SyCnt, int &UelCnt ) const;

   /**
    * @brief Finish registration of unique elements. Returns zero if the operation is not possible.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxUELRegisterRawStart, gdxUELRegisterMapStart, gdxUELRegisterStrStart
    */
   int gdxUELRegisterDone();

   /**
    * @brief Register unique element in raw mode. This can only be used while writing to a GDX file. Returns zero if the operation is not possible.
    * @details
       The unique element (UEL) is registered in raw mode, i.e. the internally
       assigned integer index is determined by the system.
       Can only be used while writing to a GDX file and gdxUELRegisterRawStart was called beforehand.
    * @param Uel String for unique element (UEL) which may not exceed 63 characters in length.
    *   Furthermore a UEL string must not mix single- and double-quotes.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxUELRegisterMap, gdxUELRegisterDone
    */
   int gdxUELRegisterRaw( const char *Uel );

   /**
    * @brief Start registering unique elements in raw mode. Returns zero if the operation is not possible.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxUELRegisterRaw, gdxUELRegisterDone
    */
   int gdxUELRegisterRawStart();

   /**
    * @brief Register a unique element in string mode. A unique element must follow the GAMS rules when it contains quote characters. Non-zero if the element was registered, zero otherwise.
    * @details
    *   The unique element is registered in user mapped space. The returned
    *   index is the next higher value. Registering an element a second time
    *   is not considered an error and the same index position will be returned.
    *   A unique element must follow the GAMS rules when it contains quote characters.
    *   Can only be used while writing to a GDX file and gdxUELRegisterStrStart was called beforehand.
    * @param Uel String for unique element (UEL) which may not exceed a length of 63 characters.
    *   Furthermore a UEL string must not mix single- and double-quotes.
    * @param UelNr Internal index number assigned to this unique element in user space (or -1 if not found).
    * @return Non-zero if the element was registered, zero otherwise.
    * @see gdxUELRegisterStrStart, gdxUELRegisterDone
    */
   int gdxUELRegisterStr( const char *Uel, int &UelNr );


   /**
    * @brief Start registering unique elements in string mode. Returns zero if the operation is not possible.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxUELRegisterStr, gdxUELRegisterDone
    */
   int gdxUELRegisterStrStart();

   /**
    * @brief Get a unique element using an unmapped index. Returns zero if the operation is not possible.
    * @param UelNr Element number (unmapped) (range 1..NrElem)  or -1 if not found.
    * @param Uel String for unique element. Buffer should be 64 bytes long (to store maximum of 63 characters).
    * @param UelMap User mapping for this element or -1 if element was never mapped.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @attention Make sure buffer for Uel is at least 64 bytes long to prevent potential overflow.
    * @see gdxUMUelInfo, gdxGetUEL
    */
   int gdxUMUelGet( int UelNr, char *Uel, int &UelMap );

   /**
    * @brief Return information about the unique elements (UELs). Always non-zero.
    * @param UelCnt Total number of unique elements (UELs in GDX file plus new registered UELs).
    * @param HighMap Highest user mapping index used.
    * @return Always returns non-zero.
    * @see gdxUMUelGet
    */
   int gdxUMUelInfo( int &UelCnt, int &HighMap ) const;

   /**
    * @brief Returns the dimension of the currently active symbol
    *    When reading or writing data, the dimension of the current active symbol
    *    is sometimes needed to convert arguments from strings to pchars (char *) etc.
    * @details
    *    When reading or writing data, the dimension of the current active symbol
    *    is sometimes needed to convert arguments from strings to pchars (char *) etc.
    *    The currently active symbol is selected e.g. by starting a write- or read-operation
    *    and choosing its symbol number (SyNr).
    * @return Dimension of current active symbol.
    * @see gdxDataReadRawStart, gdxDataWriteRawStart
    */
   [[nodiscard]] int gdxCurrentDim() const;

   /**
    * @brief Rename unique element OldName to NewName.
    * @param OldName Name of an existing unique element (UEL).
    * @param NewName New name for the UEL. Must not exist in UEL table yet.
    * @attention String for unique element (UEL) which may not exceed a length of 63 characters.
    *   Furthermore a UEL string must not mix single- and double-quotes.
    * @return Zero if the renaming was possible; non-zero is an error indicator.
    */
   int gdxRenameUEL( const char *OldName, const char *NewName );

   /**
    * @brief Open a GDX file for reading allowing for skiping sections. Non-zero if the file can be opened, zero otherwise.
    * @details
    *   Open an existing GDX file for input. If a file extension is not
    *   supplied, the extension '.gdx' will be used. The return code is
    *   a system dependent I/O error. If the file was found, but is not
    *   a valid GDX file, the function GetLastError can be used to handle
    *   these type of errors.
    * @param FileName File name of the GDX file to be opened (arbitrary length).
    * @param ReadMode Bitmap skip reading sections: 0-bit: string (1 skip reading string).
    * @param ErrNr Returns an error code or zero if there is no error.
    * @return Returns non-zero if the file can be opened; zero otherwise.
    * @see gdxOpenWrite, gdxGetLastError
    * @code
        int ErrNr;
        pgx.gdxOpenReadEx("file1.gdx", fmOpenRead, ErrNr);
        if(ErrNr) {
           [...]
        }
    * @endcode
    */
   int gdxOpenReadEx( const char *FileName, int ReadMode, int &ErrNr );

   /**
    * @brief Get the string for a unique element using a mapped index. Returns zero if the operation is not possible.
    * @details Retrieve the string for an unique element based on a mapped index number.
    * @param UelNr Index number in user space (range 1..NrUserElem).
    * @param Uel String for the unique element which may be up to 63 characters.
    * @return Return non-zero if the index is in a valid range, zero otherwise.
    * @attention Supplied buffer for storing the Uel name should be 64 bytes long to prevent overflow!
    * @see gdxUMUelGet
    */
   int gdxGetUEL( int UelNr, char *Uel ) const;

   /**
    * @brief Start writing a new symbol in mapped mode. Returns zero if the operation is not possible.
    * @param SyId Name of the symbol (up to 63 characters) or acronym.
    *   The first character of a symbol must be a letter.
    *   Following symbol characters may be letters, digits, and underscores.
    *   Symbol names must be new and unique. Might be an empty string at gdxAcronymName.
    * @param ExplTxt Explanatory text for the symbol (up to 255 characters).
    * @param Dimen Dimension of the symbol.
    * @param Typ Type of the symbol.
    * @param UserInfo User field value storing additional data; GAMS follows the following conventions:
    * <table>
    *   <thead>
    *   <tr>
    *       <th>Type</th>
    *       <th>Value(s)</th>
    *   </tr>
    *   </thead>
    *   <tbody>
    *   <tr>
    *       <td>Aliased Set</td>
    *       <td>The symbol number of the aliased set, or zero for the universe</td>
    *   </tr>
    *   <tr>
    *       <td>Set</td>
    *       <td>Zero</td>
    *   </tr>
    *   <tr>
    *       <td>Parameter</td>
    *       <td>Zero</td>
    *   </tr>
    *   <tr>
    *       <td>Variable</td>
    *       <td>The variable type: binary=1, integer=2, positive=3, negative=4, free=5, sos1=6, sos2=7, semicontinous=8,
    *           semiinteger=9
    *       </td>
    *   </tr>
    *   <tr>
    *       <td>Equation</td>
    *       <td>The equation type: eque=53, equg=54, equl=55, equn=56, equx=57, equc=58, equb=59</td>
    *   </tr>
    *   </tbody>
    * </table>
    * @return Non-zero if the operation is possible, zero otherwise
    * @see gdxDataWriteMap, gdxDataWriteDone
    */
   int gdxDataWriteMapStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );


   /**
    * @brief Write a data element in mapped mode. Returns zero if the operation is not possible.
    * @param KeyInt The index for this element using mapped values.
    * @param Values The values for this element.
    * @returnNon-zero if the operation is possible, zero otherwise.
    * @see gdxDataWriteMapStart, gdxDataWriteDone
    */
   int gdxDataWriteMap( const int *KeyInt, const double *Values );


   /**
    * @brief Initialize the reading of a symbol in mapped mode. Returns zero if the operation is not possible.
    * @param SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe.
    * @param NrRecs The maximum number of records available for reading. The actual number of records may be less when a filter is applied to the records read.
    * @returnNon-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadMap, gdxDataReadRawStart, gdxDataReadStrStart, gdxDataReadDone
    */
   int gdxDataReadMapStart( int SyNr, int &NrRecs );


   /**
    * @brief Read the next record in mapped mode. Returns zero if the operation is not possible.
    * @param RecNr Ignored (left in for backward compatibility).
    * @param KeyInt The index of the record.
    * @param Values The data of the record.
    * @param DimFrst The first index position in KeyInt that changed.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadMapStart, gdxDataReadFilteredStart, gdxDataReadDone
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
    * @brief Number of entries in the acronym table.
    * @return The number of entries in the acronym table.
    * @see gdxAcronymSetInfo, gdxAcronymSetInfo
    */
   [[nodiscard]] int gdxAcronymCount() const;


   /**
    * @brief Retrieve acronym information from the acronym table. Non-zero if the index into the acronym table is valid.
    * @param N Index into acronym table (range 1..AcronymCount).
    * @param AName Name of the acronym (up to 63 characters).
    * @param Txt Explanatory text of the acronym (up to 255 characters, mixed quote chars will be unified to first occurring quote).
    * @param AIndx Index value of the acronym.
    * @return Non-zero if the index into the acronym table is valid; false otherwise.
    * @attention Make sure AName is 64 bytes and Txt 256 bytes wide to prevent overflow!
    * @see gdxAcronymSetInfo, gdxAcronymCount
    */
   int gdxAcronymGetInfo( int N, char *AName, char *Txt, int &AIndx ) const;


   /**
    * @brief Modify acronym information in the acronym table
    *   <ul><li>When writing a GDX file, this function is used to provide the name of an acronym;
    *     in this case the Indx parameter must match.</li>
    *   <li>When reading a GDX file, this function
    *     is used to provide the acronym index, and the AName parameter must match.</li></ul>
    * @param N Index into acronym table (range 1..AcronymCount).
    * @param AName Name of the acronym (up to 63 characters).
    *   The first character of a symbol must be a letter.
    *   Following symbol characters may be letters, digits, and underscores.
    *   Symbol names must be new and unique.
    * @param Txt Explanatory text of the acronym (up to 255 characters, mixed quote chars will be unified to first occurring quote).
    * @param AIndx Index value of the acronym.
    * @return Non-zero if the index into the acronym table is valid; false otherwise.
    * @see gdxAcronymGetInfo, gdxAcronymCount
    */
   int gdxAcronymSetInfo( int N, const char *AName, const char *Txt, int AIndx );


   /**
    * @brief Returns the value of the NextAutoAcronym variable and sets the variable to nv.
    *    <ul><li>When we read from a GDX file and encounter an acronym that was not defined, we need to assign
    *    a new index for that acronym. The variable NextAutoAcronym is used for this purpose and is
    *    incremented for each new undefined acronym.</li>
    *    <li>When NextAutoAcronym has a value of zero, the default, the value is ignored and the original
    *    index as stored in the GDX file is used for the index.</li></ul>
    * @param NV New value for NextAutoAcronym; a value of less than zero is ignored.
    * @return Previous value of NextAutoAcronym.
    */
   int gdxAcronymNextNr( int NV );


   /**
    * @brief Get information how acronym values are remapped. When reading GDX data, we need to map indices for acronyms used in the GDX file to indices used by the reading program. Non-zero if the index into the acronym table is valid.
    * @details
    *   When reading GDX data, we need to map indices for acronyms used in the GDX file to
    *   indices used by the reading program. There is a problem when not all acronyms have been
    *   registered before reading the GDX data. We need to map an undefined index we read to a new value.
    *   The value of NextAutoAcronym is used for that.
    * @param N Index into acronym table; range from 1 to AcronymCount.
    * @param orgIndx The Index used in the GDX file.
    * @param newIndx The Index returned when reading GDX data.
    * @param autoIndex Non-zero if the newIndx was generated using the value of NextAutoAcronym.
    * @return Non-zero if the index into the acronym table is valid; false otherwise.
    * @see gdxAcronymGetInfo, gdxAcronymCount, gdxAcronymNextNr
    */
   int gdxAcronymGetMapping( int N, int &orgIndx, int &newIndx, int &autoIndex );
   // endregion

   // region Filter handling
   /**
    * @brief Check if there is a filter defined based on its number as used in gdxFilterRegisterStart. Returns zero if the operation is not possible.
    * @param FilterNr Filter number as used in FilterRegisterStart.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxFilterRegisterStart
    */
   int gdxFilterExists( int FilterNr );

   /**
    * @brief Define a unique element filter. Returns zero if the operation is not possible.
    * @details
    *   Start the registration of a filter. A filter is used to map a number
    *   of elements to a single integer; the filter number. A filter number
    *   can later be used to specify a filter for an index postion when reading data.
    * @param FilterNr Filter number to be assigned.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxFilterRegister, gdxFilterRegisterDone, gdxDataReadFilteredStart
    */
   int gdxFilterRegisterStart( int FilterNr );

   /**
    * @brief Add a unique element to the current filter definition, zero if the index number is out of range or was never mapped into the user index space.
    * @details
       Register a unique element as part of the current filter. The
       function returns false if the index number is out of range of
       valid user indices or the index was never mapped into the
       user index space.
    * @param UelMap Unique element number in the user index space or -1 if element was never mapped.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxFilterRegisterStart, gdxFilterRegisterDone
    */
   int gdxFilterRegister( int UelMap );

   /**
    * @brief Finish registration of unique elements for a filter. Returns zero if the operation is not possible.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxFilterRegisterStart, gdxFilterRegister
    */
   int gdxFilterRegisterDone();

   /**
    * @brief Initialize the reading of a symbol in filtered mode. Returns zero if the operation is not possible.
    * @param SyNr The index number of the symbol, range 0..NrSymbols; SyNr = 0 reads universe.
    * @param FilterAction Array of filter actions for each index position.
    * @param NrRecs The maximum number of records available for reading. The actual number of records may be
    *      less when a filter is applied to the records read.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxFilterRegisterStart, gdxDataReadMap, gdxDataReadRawStart, gdxDataReadStrStart, gdxDataReadDone
    * @details
    *   <p>Start reading data for a symbol in filtered mode. Each filter action
    *   (1..Dimension) describes how each index should be treated when reading
    *   a data record. When new unique elements are returned, they are added
    *   to the user index space automatically. The actual reading of records
    *   is done with DataReadMap.</p>
    *   <p>The action codes are as follows:
    *     <table>
    *     <thead>
    *     <tr>
    *       <th>Action code</th>
    *       <th>Result</th>
    *     </tr>
    *     </thead>
    *     <tbody>
    *     <tr>
    *       <td>DOMC_UNMAPPED</td>
    *       <td>The index is not mapped into user space</td>
    *     </tr>
    *     <tr>
    *       <td>DOMC_EXPAND</td>
    *       <td>New unique elements encountered will be mapped into the user space</td>
    *     </tr>
    *     <tr>
    *       <td>DOMC_STRICT</td>
    *       <td>If the unique element in this position does not map into user space, the record will not be available and is
    *           added to the error list instead
    *       </td>
    *     </tr>
    *     <tr>
    *       <td>FilterNumber</td>
    *       <td>If the unique element in this position does not map into user space or is not enabled in this filter, the
    *           record will not be available and is added to the error list instead
    *       </td>
    *     </tr>
    *     </tbody>
    *     </table>
    *   </p>
*/
   int gdxDataReadFilteredStart( int SyNr, const int *FilterAction, int &NrRecs );
   // endregion

   /**
    * @brief Set the Node number for an entry in the string table. After registering a string with AddSetText, we can assign a node number for later retrieval. Returns zero if the operation is not possible.
    * @details
       After registering a string with AddSetText, we can assign
       a node number for later retrieval. The node number is any
       integer which is stored without further restrictions.
    * @param TxtNr Index number of the entry to be modified.
    * @param Node The new Node value for the entry.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxAddSetText, gdxGetElemText
    */
   int gdxSetTextNodeNr( int TxtNr, int Node );


   /**
    * @brief Get the unique elements for a given dimension of a given symbol.
    * @details
    *   Using the data of a symbol, get the unique elements for a given index position. To achieve this,
    *   the symbols data is read and a tally is kept for the elements in the given index position. When a filter
    *   is specified, records that have elements in the specified index position that are outside the filter will
    *   be added to the list of DataErrorRecords. See gdxDataErrorRecord.
    * @param SyNr The index number of the symbol, range 1..NrSymbols; SyNr = 0 reads universe.
    * @param DimPos  The dimension to use, range 1..dim.
    * @param FilterNr Number of a previously registered filter or the value DOMC_EXPAND if no filter is wanted.
    * @param DP Callback procedure which will be called once for each available element (can be nil).
    * @param NrElem Number of unique elements found.
    * @param Uptr User pointer; will be passed to the callback procedure.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @code
        int Cnt;
        auto DataDomainCB = [](int RawNr, int MappedNr, void *UPtr) {
            std::cout << RawNr << " (" << MappedNr << ")" << std::endl;
        };
        pgx.gdxGetDomainElements(1, 1, DOMC_EXPAND, nullptr, cnt);
        std::cout << "Domain count only = " << cnt << std::endl;
        pgx.gdxGetDomainElements(1, 1, DOMC_EXPAND, DataDomainCB, cnt);
        std::cout << "Get domain count = " << cnt << std::endl;
        pgx.gdxGetDomainElements(1, 1, 7, DataDomainCB, cnt);
        std::cout << "Using filter 7; number of records in error list = " << gdxDataErrorCount(PGX) << std::endl;
    * @endcode
    */
   int gdxGetDomainElements( int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int &NrElem, void *Uptr );

   /**
    * @brief Set the amount of trace (debug) information generated. Always non-zero.
    * @param N Tracing level  N <= 0 no tracing  N >= 3 maximum tracing.
    * @param s A string to be included in the trace output (arbitrary length).
    * @return Always non-zero.
    */
   int gdxSetTraceLevel( int N, const char *s );


   /**
    * @brief Add a new acronym entry. This can be used to add entries before data is written. Returns negative value (<0) if the entry is not added.
    * @details
    *   This function can be used to add entries before data is written. When entries
    *   are added implicitly use gdxAcronymSetInfo to update the table.
    * @param AName Name of the acronym (up to 63 characters)
    *   The first character of a symbol must be a letter.
    *   Following symbol characters may be letters, digits, and underscores.
    *   Symbol names must be new and unique.
    * @param Txt Explanatory text of the acronym (up to 255 characters, mixed quotes will be unified to first occurring quote character).
    * @param AIndx Index value of the acronym.
    * @return
    *   <ul><li>0 If the entry is not added because of a duplicate name using the same value fo the index.</li>
    *   <li>-1 If the entry is not added because of a duplicate name using a different value for the index.</li>
    *   <li>Otherwise the index into the acronym table (1..gdxAcronymCount).</li></ul>
    * @see gdxAcronymGetInfo, gdxAcronymCount
    */
   int gdxAcronymAdd( const char *AName, const char *Txt, int AIndx );

   /**
    * @brief Get index value of an acronym. Returns zero if V does not represent an acronym.
    * @param V Input value possibly representing an acronym/Version string after return (gdxGetDLLVersion).
    * @return Index of acronym value V; zero if V does not represent an acronym.
    * @see gdxAcronymValue
    */
   [[nodiscard]] int gdxAcronymIndex( double V ) const;

   /**
    * @brief Find the name of an acronym value. Non-zero if a name for the acronym is defined. An unnamed acronym value will return a string of the form UnknownAcronymNNN, were NNN is the index of the acronym.
    * @param V Input value possibly containing an acronym/Version string after return (gdxGetDLLVersion).
    * @param AName Name of acronym value or the empty string (can be up to 63 characters).
    * @return
    *   Return non-zero if a name for the acronym is defined. Return
    *   zero if V does not represent an acronym value or a name
    *   is not defined. An unnamed acronym value will return a string
    *   of the form UnknownAcronymNNN; were NNN is the index of the acronym.
    * @attention Supplied buffer for AName should be 64 bytes long to prevent overflow!
    * @see gdxAcronymIndex
    */
   int gdxAcronymName( double V, char *AName );

   /**
    * @brief Create an acronym value based on the index (AIndx should be greater than 0). Returns the calculated acronym value (zero if AIndx is <0).
    * @param AIndx Index value; should be greater than zero.
    * @return The calculated acronym value; zero if Indx is not positive.
    * @see gdxAcronymIndex
    */
   [[nodiscard]] double gdxAcronymValue( int AIndx ) const;

   /**
    * @brief Returns the value of the AutoConvert variable and sets the variable to nv.
    *   <p>When we close a new GDX file, we look at the value of AutoConvert; if AutoConvert
    *   is non-zero, we look at the GDXCOMPRESS and GDXCONVERT environment variables to determine if
    *   conversion to an older file format is desired. We needed this logic so gdxcopy.exe
    *   can disable automatic file conversion.</p>
    * @param NV New value for AutoConvert.
    * @return Previous value of AutoConvert.
    */
   int gdxAutoConvert( int NV );

   /**
    * @brief Returns a version descriptor of the library. Always non-zero.
    * @param V Contains version string after return.
    * @return Always returns non-zero.
    * @attention Output argument buffer V should be 256 bytes long.
    */
   static int gdxGetDLLVersion( char *V );


   /**
    * @brief Returns file format number and compression level used. Always non-zero.
    * @param FileVer File format number or zero if the file is not open.
    * @param ComprLev Compression used; 0= no compression, 1=zlib.
    * @return Always returns non-zero.
    */
   int gdxFileInfo( int &FileVer, int &ComprLev ) const;

   /**
    * @brief Prepare for the reading of a slice of data from a data set. The actual read of the data is done by calling gdxDataReadSlice. When finished reading, call gdxDataReadDone. Returns zero if the operation is not possible.
    * @param SyNr Symbol number to read, range 1..NrSymbols; SyNr = 0 reads universe.
    * @param ElemCounts Array of integers, each position indicating the number of
    *              unique indices in that position.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadSlice, gdxDataReadDone
    */
   int gdxDataReadSliceStart( int SyNr, int *ElemCounts );

   /**
    * @brief Read a slice of data from a data set, by fixing zero or more index positions in the data.
    *   When a data element is available, the callback procedure DP is called with the
    *   current index and the values. The indices used in the index vary from zero to
    *   the highest value minus one for that index position. This function can be called
    *   multiple times. Returns zero if the operation is not possible.
    * @param UelFilterStr Each index can be fixed by setting the string for the unique
    *                   element. Set an index position to the empty string in order
    *                   not to fix that position.
    * @param Dimen The dimension of the index space; this is the number of index positions that is not fixed.
    * @param DP Callback procedure which will be called for each available data item.
    *   Signature is
    *       <ul><li>UEL index number keys for each symbol dimension (const int *)</li>
    *       <li>5 double values (const double *)</li></ul>
    * @return Non-zero if the operation is possible, zero otherwise.
    * @attention Supply one UEL filter str for each symbol dimension (up to 63 characters per str).
    * @see gdxDataReadSliceStart, gdxDataSliceUELS, gdxDataReadDone
    */
   int gdxDataReadSlice( const char **UelFilterStr, int &Dimen, TDataStoreProc_t DP );

   /**
    * @brief Map a slice index in to the corresponding unique elements.
    *   After calling DataReadSliceStart, each index position is mapped from 0 to N(d)-1.
    *   This function maps this index space back in to unique elements represented as
    *   strings. Returns zero if the operation is not possible.
    * @param SliceKeyInt The slice index to be mapped to strings with one entry for each symbol dimension.
    * @param KeyStr Array of strings containing the unique elements.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @attention Both SliceKeyInt and KeyStr should match the symbol dimension with their length
    *   The string buffers pointed to by KeyStr should each be at least 64 bytes long to store up to 63 character UEL names.
    * @see gdxDataReadSliceStart, gdxDataReadDone
    */
   int gdxDataSliceUELS( const int *SliceKeyInt, char **KeyStr );

   /**
    * @brief Return the number of bytes used by the data objects.
    * @return The number of bytes used by the data objects.
    */
   int64_t gdxGetMemoryUsed();

   /**
    * @brief Classify a value as a potential special value. Non-zero if D is a special value, zero otherwise.
    * @param D Value to classify.
    * @param sv Classification.
    * @return Returns non-zero if D is a special value, zero otherwise.
    * @see gdxGetSpecialValues, gdxSetSpecialValues
    */
   int gdxMapValue( double D, int &sv );

   /**
    * @brief Open an existing GDX file for output. Non-zero if the file can be opened, zero otherwise.
    * @details
    *   If a file extension is not supplied, the extension '.gdx' will be used.
    *   The return code is a system dependent I/O error.
    *   When appending to a GDX file, the symbol table, uel table etc will be read
    *   and the whole setup will be treated as if all symbols were just written to
    *   the GDX file. Replacing a symbol is not allowed; it will generate a duplicate
    *   symbol error.
    * @param FileName File name of the GDX file to be created (arbitrary length).
    * @param Producer Name of program that appends to the GDX file (should not exceed 255 characters).
    * @param ErrNr Returns an error code or zero if there is no error.
    * @return Returns non-zero if the file can be opened; zero otherwise.
    * @see gdxOpenRead, gdxOpenWrite, gdxOpenWriteEx
    * @code
        std::string Msg;
        TGXFileObj pgx {Msg}
        int ErrCode;
        pgx.gdxOpenAppend("file1.gdx","Examples", ErrCode);
        if(ErrCode) {
          [ ... ]
        }
    * @endcode
    */
   int gdxOpenAppend( const char *FileName, const char *Producer, int &ErrNr );

   /**
    * @brief Test if any of the elements of the set has an associated text. Non-zero if the Set contains at least one unique element that has associated text, zero otherwise.
    * @param SyNr Set symbol number (range 1..NrSymbols); SyNr = 0 reads universe.
    * @return Non-zero if the set contains at least one element that has associated text, zero otherwise.
    * @see gdxSystemInfo, gdxSymbolInfo
    */
   int gdxSetHasText( int SyNr );

   /**
    * @brief Set the internal values for special values when reading a GDX file. Before calling this function, initialize the array of special values by calling gdxGetSpecialValues first. Always non-zero.
    * @param AVals 7-element array of special values to be used for Eps, +Inf, -Inf, NA and Undef. Note that the values do not have to be unique.
    * @return Always non-zero.
    * @note Before calling this function, initialize the array of special values by calling gdxGetSpecialValues first.
    * @attention AVals must have length of at least 7 double values.
    * @see gdxSetSpecialValues, gdxResetSpecialValues, gdxGetSpecialValues
    */
   int gdxSetReadSpecialValues( const double *AVals );

   /**
    * @brief Returns the length of the longest UEL used for every index position for a given symbol.
    * @param SyNr Symbol number (range 1..NrSymbols); SyNr = 0 reads universe.
    * @param LengthInfo The longest length for each index position. This output argument should be able to store one integer for each symbol dimension.
    * @return The length of the longest UEL found in the data (over all dimensions).
    * @attention Make sure LengthInfo can store one integer for each symbol dimension.
    * @see gdxUELMaxLength
    */
   int gdxSymbIndxMaxLength( int SyNr, int *LengthInfo );

   /**
    * @brief Returns the length of the longest symbol name in the GDX file.
    * @return The number of characters in the longest symbol name.
    */
   [[nodiscard]] int gdxSymbMaxLength() const;

   /**
    * @brief Add a line of comment text for a symbol. Returns zero if the operation is not possible.
    * @param SyNr The symbol number (range 1..NrSymbols); if SyNr <= 0 the current symbol being written.
    * @param Txt String to add which should not exceed a length of 255 characters.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @attention A line of comment text can not contain more than 255 characters but there are no other restrictions on it.
    * @see gdxSymbolGetComment
    */
   int gdxSymbolAddComment( int SyNr, const char *Txt );

   /**
    * @brief Retrieve a line of comment text for a symbol. Returns zero if the operation is not possible.
    * @param SyNr The symbol number (range 1..NrSymbols); SyNr = 0 reads universe.
    * @param N Line number in the comment block (1..Count).
    * @param Txt String containing the line requested (empty on error). Buffer should be able to hold 255 characters.
    *   Potential causes for empty strings are symbol- (SyNr) or line-number (N) out of range.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @attention Output argument string buffer must have size of 256 bytes.
    * @see gdxSymbolAddComment
    */
   int gdxSymbolGetComment( int SyNr, int N, char *Txt );

   /**
    * @brief Returns the length of the longest unique element (UEL) name.
    * @return The length of the longest UEL name in the UEL table.
    * @see gdxSymbIndxMaxLength
    */
   [[nodiscard]] int gdxUELMaxLength() const;

   /**
    * @brief Search for unique element by its string. Non-zero if the element was found, zero otherwise.
    * @param Uel String to be searched (not longer than 63 characters, don't mix single- and double-quotes).
    * @param UelNr Internal unique element number or -1 if not found.
    * @param UelMap User mapping for the element or -1 if not found or the element was never mapped.
    * @return Non-zero if the element was found, zero otherwise.
    */
   int gdxUMFindUEL( const char *Uel, int &UelNr, int &UelMap );

   [[nodiscard]] int gdxStoreDomainSets() const;
   void gdxStoreDomainSetsSet( int x );

   /**
    * @brief Read a symbol in Raw mode while applying a filter using a callback procedure. Returns zero if the operation is not possible.
    * @details
    *   Read a slice of data, by fixing zero or more index positions in the data.
    *   When a data element is available, the callback procedure DP is called with the
    *   current index (as raw numbers) and the values.
    * @param SyNr The index number of the symbol, range from 0 to NrSymbols; SyNr = 0 reads universe.
    * @param UelFilterStr Each index can be fixed by setting the string for the unique
    *                 element. Set an index position to the empty string in order
    *                 not to fix that position. If the string is not-empty it should match
    *                 an UEL name from the UEL table.
    * @param DP Callback procedure which will be called for each available data item.
    *   This procedure (return type=void) should have the following signature:
    *       <ul><li>UEL index number keys (const int *),</li>
    *       <li>values (level, marginal, lower-, upper-bound, scale) (const double *),</li>
    *       <li>pointer to custom data (void *).</li></ul>
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadRawFast, gdxDataReadSliceStart, gdxDataSliceUELS, gdxDataReadDone,
    * @code
      auto DPCallBack = [](const int *Indx, const double *Vals, void *Uptr)
      {
        std::string s;
        int UelMap;
        ((TGXFileObj*)Uptr).gdxUMUelGet(Indx[1], s, UelMap);
        std::cout << s << ' ' << Vals[vallevel] << std::endl;
        return 1;
      };
      TgdxStrIndex IndxS;
      IndxS[0] = "i200"; IndxS[1] = "";
      pgx.gdxDataReadRawFastFilt(1, IndxS, DPCallBack);
    * @endcode
    */
   int gdxDataReadRawFastFilt( int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP );

   /**
    * @brief Read a symbol in Raw mode using a callback procedure. Returns zero if the operation is not possible.
    * @details
    *   Use a callback function to read a symbol in raw mode. Using a callback procedure
    *   to read the data is faster because we no longer have to check the context for each
    *   call to read a record.
    * @param SyNr The index number of the symbol (range 0..NrSymbols); SyNr = 0 reads universe.
    * @param DP Procedure that will be called for each data record.
    *   This procedure (return type=void) should have the following signature:
    *       <ul><li>UEL index number keys (const int *),</li>
    *       <li>values (level, marginal, lower-, upper-bound, scale) (const double *)</li></ul>
    * @param NrRecs The maximum number of records available for reading. The actual number of records may be less when a filter is applied to the records read.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadRaw, gdxDataReadMapStart, gdxDataReadStrStart, gdxDataReadDone, gdxDataReadRawFastFilt
    */
   int gdxDataReadRawFast( int SyNr, TDataStoreProc_t DP, int &NrRecs );

   /**
    * @brief Read a symbol in Raw mode using a callback procedure. Returns zero if the operation is not possible.
    * @details
    *   Use a callback function to read a symbol in raw mode. Using a callback procedure
    *   to read the data is faster because we no longer have to check the context for each
    *   call to read a record.
    * @param SyNr The index number of the symbol (range 0..NrSymbols); SyNr = 0 reads universe.
    * @param DP Procedure that will be called for each data record.
    *   This procedure (return type=void) should have the following signature:
    *       <ul><li>UEL index number keys (const int *),</li>
    *       <li>values (level, marginal, lower-, upper-bound, scale) (const double *),</li>
    *       <li>dimension of first change (int),</li>
    *       <li>pointer to custom data (void *)</li>
    *       </ul>
    * @param NrRecs The number of records available for reading.
    * @param Uptr Pointer to user memory that will be passed back with the callback.
    * @return Non-zero if the operation is possible, zero otherwise.
    * @see gdxDataReadRawFast
    */
   int gdxDataReadRawFastEx( int SyNr, TDataStoreExProc_t DP, int &NrRecs, void *Uptr );

private:
   std::unique_ptr<gmsstrm::TMiBufferedStreamDelphi> FFile;
   TgxFileMode fmode { f_not_open }, fmode_AftReg { f_not_open };
   enum
   {
      stat_notopen,
      stat_read,
      stat_write
   } fstatus { stat_notopen };
   int fComprLev {};
   std::unique_ptr<TUELTable> UELTable;
   std::unique_ptr<TSetTextList> SetTextList {};
   std::unique_ptr<int[]> MapSetText {};
   int FCurrentDim {};
   std::array<int, GLOBAL_MAX_INDEX_DIM> LastElem {}, PrevElem {}, MinElem {}, MaxElem {};
   std::array<std::array<char, GLOBAL_UEL_IDENT_SIZE>, GLOBAL_MAX_INDEX_DIM> LastStrElem {};
   int DataSize {};
   tvarvaltype LastDataField {};
   std::unique_ptr<TNameList> NameList;
   std::unique_ptr<TDomainStrList> DomainStrList;
   std::unique_ptr<LinkedDataType> SortList;
   std::optional<LinkedDataIteratorType> ReadPtr;
   std::unique_ptr<TTblGamsDataImpl<double>> ErrorList;
   PgdxSymbRecord CurSyPtr {};
   int ErrCnt {}, ErrCntTotal {};
   int LastError {}, LastRepError {};
   std::unique_ptr<TFilterList> FilterList;
   TDFilter *CurFilter {};
   TDomainList DomainList {};
   bool StoreDomainSets { true };
   TIntlValueMapDbl intlValueMapDbl {}, readIntlValueMapDbl {};
   TIntlValueMapI64 intlValueMapI64 {};
   TraceLevels TraceLevel { TraceLevels::trl_all };
   std::string TraceStr;
   int VersionRead {};
   std::string FProducer, FProducer2, FileSystemID;
   int64_t MajorIndexPosition {};
   int64_t NextWritePosition {};
   int DataCount {}, NrMappedAdded {};
   std::array<TgdxElemSize, GLOBAL_MAX_INDEX_DIM> ElemType {};
   std::string MajContext;
   std::array<std::optional<TIntegerMapping>, GLOBAL_MAX_INDEX_DIM> SliceIndxs, SliceRevMap;
   int SliceSyNr {};
   std::array<std::string, GMS_MAX_INDEX_DIM> SliceElems;
   bool DoUncompress {},  // when reading
           CompressOut {};// when writing
   int DeltaForWrite {};  // delta for last dimension or first changed dimension
   int DeltaForRead {};   // first position indicating change
   double Zvalacr {};     // tricky
   std::unique_ptr<TAcronymList> AcronymList;
   std::array<TSetBitMap *, GLOBAL_MAX_INDEX_DIM> WrBitMaps {};
   bool ReadUniverse {};
   int UniverseNr {}, UelCntOrig {};// original uel count when we open the file
   int AutoConvert { 1 };
   int NextAutoAcronym {};
   bool AppendActive {};

#ifndef VERBOSE_TRACE
   const TraceLevels defaultTraceLevel { TraceLevels::trl_none };
   const bool verboseTrace {};
#else
   const TraceLevels defaultTraceLevel { TraceLevels::trl_all };
   const bool verboseTrace { true };
#endif

   //api wrapper magic for Fortran
   TDataStoreFiltProc_t gdxDataReadRawFastFilt_DP {};
   TDomainIndexProc_t gdxGetDomainElements_DP {};

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
   bool gdxGetDomainElements_DP_CallByRef {},
           gdxDataReadRawFastFilt_DP_CallByRef {},
           gdxDataReadRawFastEx_DP_CallByRef {};
};

}// namespace gdx
