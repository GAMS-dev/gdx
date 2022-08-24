#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "mistream.h"

namespace simplegdx {
namespace special_value_defaults {
static const double kValUnd = 1.0E300;   // undefined
static const double kValNa = 2.0E300;    // not available/applicable
static const double kValPin = 3.0E300;   // plus infinity
static const double kValMin = 4.0E300;   // minus infinity
static const double kValEps = 5.0E300;   // epsilon
static const double kValAcr = 10.0E300;  // potential/real acronym
}  // namespace special_value_defaults

namespace symbol_type {
static const uint8_t kSet = 0;
static const uint8_t kParameter = 1;
static const uint8_t kVariable = 2;
static const uint8_t kEquation = 3;
static const uint8_t kAlias = 4;
}  // namespace symbol_type

namespace exceptions {
struct WrongGDXHeaderException : public std::exception {
  const char *what() { return "Unknown GDX Header"; }
};

struct IncorrectMarkerException : public std::exception {
  const char *what() { return "Unknown GDX Marker"; }
};

struct IncorrectMode : public std::exception {
  const char *what() { return "Tried to read while creating or vice-versa"; }
};

}  // namespace exceptions

class SymbolDeltaTypeArray {
 public:
  static constexpr uint8_t kUnknown = 0;
  static constexpr uint8_t kByte = 1;
  static constexpr uint8_t kWord = 2;
  static constexpr uint8_t kInteger = 3;
  SymbolDeltaTypeArray() : val_(0) {}
  void setDeltaType(uint8_t dimension, uint8_t type);
  uint8_t getDeltaType(uint8_t dimension);

 private:
  int64_t val_;
};

struct SymbolRaw {
  SymbolRaw(const std::string &name, uint8_t symbol_type, uint8_t dimension)
      : name(name),
        position(-1),
        dimension(dimension),
        type(symbol_type),
        user_info(0),
        data_count(0),
        error_count(0),
        has_set_text(false),
        explanatory_text(""),
        domain_controlled(0),
        comment_count(0),
        domains(),
        comments() {}

  SymbolRaw()
      : name(""),
        position(-1),
        dimension(0),
        type(0),
        user_info(0),
        data_count(0),
        error_count(0),
        has_set_text(false),
        explanatory_text(""),
        domain_controlled(0),
        comment_count(0),
        domains(),
        comments() {}

  std::string name;
  int64_t position;
  int32_t dimension;
  uint8_t type;
  int32_t user_info;
  int32_t data_count;
  int32_t error_count;
  uint8_t has_set_text;
  std::string explanatory_text;
  // uint8_t compressed;
  uint8_t domain_controlled;
  int32_t comment_count;
  std::vector<int> domains;
  std::vector<std::string> comments;
};

struct AcronymRaw {
  AcronymRaw(const std::string &name, const std::string &explanatory_text,
             int32_t map)
      : name(name), explanatory_text(explanatory_text), map(map){};
  AcronymRaw() : name(), explanatory_text(), map() {}
  std::string name;
  std::string explanatory_text;
  int32_t map;
};

struct RelaxedDomainsRaw {
  std::vector<std::string> used_domain_symbol_names;
  std::map<int, std::vector<int>> symbol_index_to_domain_indexes;
};

class GAMSDataExchange {
 private:
  struct DataSectionInfo {
    uint8_t value_dimension;
    uint8_t key_dimension;
    int32_t min_key[20];
    int32_t last_key[20];
    SymbolDeltaTypeArray symbol_delta_type_arr;
    bool written;
  };

  struct SymbolSectionInfo {
    int32_t promised_symbol_count;
    int32_t symbol_count;
  };

  union Marker {
    struct {
      int64_t symbol_position;
      int64_t uel_position;
      int64_t set_text_position;
      int64_t acronym_position;
      int64_t next_write_position;
      int64_t relaxed_domain_position;
    } n;
    struct {
      int64_t positions[6];
    } a;
  };

  enum GDXValType {
    vm_valund = 0,
    vm_valna = 1,
    vm_valpin = 2,
    vm_valmin = 3,
    vm_valeps = 4,
    vm_zero = 5,
    vm_one = 6,
    vm_mone = 7,
    vm_half = 8,
    vm_two = 9,
    vm_normal = 10
  };

  static constexpr int32_t k_Mark_BOI = 19510624;
  static const constexpr char *k_Mark_UEL = "_UEL_";
  static const constexpr char *k_Mark_SYMB = "_SYMB_";
  static const constexpr char *k_Mark_DATA = "_DATA_";
  static const constexpr char *k_Mark_SETT = "_SETT_";
  static const constexpr char *k_Mark_ACRO = "_ACRO_";
  static const constexpr char *k_Mark_DOMS = "_DOMS_";

  enum GDXWriteState {
    kInit = static_cast<uint8_t>(0),
    kWritingRecords = static_cast<uint8_t>(1),
    kWritingSymbols = static_cast<uint8_t>(2),
    kWritingSetTexts = static_cast<uint8_t>(3),
    kWritingUELs = static_cast<uint8_t>(4),
    kWritingAcronyms = static_cast<uint8_t>(5),
    kWritingRelaxedDomainSetNames = static_cast<uint8_t>(6),
    kWritingRelaxedDomains = static_cast<uint8_t>(7),
  };

  static constexpr uint8_t kGDXHeaderNr = 123;
  static const constexpr char *kGDXHeaderId = "GAMSGDX";

 private:
  mistream::EndianCorrectedStream ec_stream_;
  bool compressed_;
  std::string producer_;
  std::string audit_;
  int32_t file_GDX_version;

 public:
  GAMSDataExchange(const std::string &fileName, const std::string &producer,
                   bool compr);
  explicit GAMSDataExchange(const std::string &fileName);
  ~GAMSDataExchange();

 public:
  static constexpr int32_t kGDX_Version = 7;
  static const constexpr char *kAuditTest =
      "GDX Library      30.2.0 r482c588 Released Feb 07, 2020 LEG x86 "
      "64bit/Linux    ";

  /**
  @return true if compression is used in the GDX file else false
  */
  inline bool IsCompressed() { return compressed_; }

  /**
  @return a new string containing producer
  */
  std::string GetProducer() { return producer_; }

  /**
  @return a new string containing auidit
  */
  std::string GetAudit() { return audit_; }

  /**
  @return an integer containing GDX File Version
  */
  int32_t GetFileGDXVersion() { return file_GDX_version; }

  /**
  @return an integer containing Library GDX Version
  */
  static constexpr int32_t GetLibraryGDXVersion() { return kGDX_Version; }

  /**
  Goes to symbols mark in GDX stream
  @return Number of the symbol
  */
  int32_t ReadSymbolHeader();

  /**
  Reads the next symbol information,
  Returns an empty symbol if the end is reached.
  Compare the return symbol's dimension to -1 to verify.
  @return Symbol information
  */
  SymbolRaw ReadSymbol();

  /**
   Switches the cursor to records of a symbol
   @return Returns if jump is successful
   that is required for ReadRecord
   */
  bool ReadRecordHeader(const SymbolRaw &symbol);

  /**
  Reads the next record of the symbol.
  @return false if no record was left or ReadRecordHeader wasn't called first
  @return true if a record is read
  */
  bool ReadRecord(int32_t *keyArr, double *valArr);

  /**
  Read Set texts into a vector
  @return std::vector<std::string> a vector contains the set texts
  */
  std::vector<std::string> ReadSetTexts();

  /**
  Reads Set Texts Header and return the number of Set Texts.
  This Function should be called before calling ReadSetText.
  */
  int32_t ReadSetTextHeader();

  /**
  Reads the next set text. Before calling this function,
  ReadSetTextHeader should be called.

  Returns true if the set text is read otherwise false
  */
  bool ReadSetText(std::string &str);

  /**
  Reads the UEL header and returns the number of UELs.
  This function should be called before ReadUEL.
  */
  int32_t ReadUELHeader();

  /**
  Reads a single UEL and puts it into str.
  If there is no UELs left or ReadUELHeader is not called before
  it returns false and does not modify str.

  Calling ReadUELHeader resets reading to the beginning of UELs.
  */
  bool ReadUEL(std::string &str);

  /**
  Read UELS into a vector
  @return std::vector<std::string> a vector contains the UELS
  */
  std::vector<std::string> ReadUELs();

  /**
  Reads the Acronyms Header and returns the number of Acronyms
  This function should be called before calling ReadAcronym
  */
  int32_t ReadAcronymHeader();

  /**
  Reads the next Acronym and fills name,  explanatory_text and acronym_index
  Returns true if there was an acronym, false if there was no acronyms or
  ReadAcronymHeader was not called before.
  */
  bool ReadAcronym(std::string &name, std::string &explanatory_text,
                   int &acronym_index);

  /**
  Reads the next Acronym and fills acronym
  Returns true if there was an acronym, false if there was no acronyms or
  ReadAcronymHeader was not called before.
 */
  bool ReadAcronym(AcronymRaw &acronym);

  /**
   Read Acronyms into a vector
   @return std::vector<AcronymRaw> a vector contains the Acronyms
   */
  std::vector<AcronymRaw> ReadAcronyms();

  /**
  Reads Relaxed Domains header, and returns number of used sets.
   */
  int32_t ReadRelaxedDomainsHeader();

  /**
  Fills the symbol number and domain_set_name_array with the next
  relaxed domain symbol. Before calling this function, ReadRelaxedDomainSetName
  should be exhausted(called until it returns false)
   */
  bool ReadRelaxedDomainSymbol(int &symbol_number, int dim,
                               int *domain_set_name_array);

  /**
  Reads the next set name used in the relaxed domains.
  Before calling this function ReadRelaxedDomainsHeader must be called.
  Returns true if there was a next set name, false otherwise.
  When it returns false, user should continue to ReadRelaxedDomainSymbol
  */
  bool ReadRelaxedDomainSetName(std::string &name);

  /**
  Reads domain information set by gdxSymbolSetDomainX
  @return pair<vector<string>, map<int, vector<int>>>
  First element of the pair contains symbol names for domains
  Second element of the pair contains affected symbol index as key
  and symbol name indices for each dimension of the that symbol.
  Indices are valid for first element.
  */
  RelaxedDomainsRaw ReadRelaxedDomains(const std::vector<uint8_t> &dimensions);

  void ReadSystemInfo(int32_t *number_symbols, int32_t *number_uels);

  /**
  Writes header for a data section. This function must be called before
  calling WriteRecord.
  @return the position of the current section
  */
  int64_t WriteRecordHeader(SymbolRaw &symbol, const int32_t *min_key,
                            const int32_t *max_key);

  /**
  Writes a record. Before calling this function, WriteRecordHeader must be
  called.
  @return true if the writing is successful, else returns false
   */
  bool WriteRecord(simplegdx::SymbolRaw &symbol, const int32_t *key_arr,
                   double *val_arr);

  /**
  Writes header for the symbol table section.
  This function must be called before calling WriteSymbol.
  @return the position of the symbol section if succesful else returns -1
  */
  int64_t WriteSymbolHeader(int32_t number_of_symbols);

  /**
 Writes a symbol. Before calling this,
 WriteSymbolHeader function should be called.
 @return true if the writing is successful, else returns false
  */
  bool WriteSymbol(const SymbolRaw &symbol);

  /**
  Writes symbols. If WriteSymbols is used, you should not use
  WriteSymbolHeader and WriteSymbol
  @return the position of the symbol section if succesful else returns -1
   */
  int64_t WriteSymbols(const std::vector<SymbolRaw> &symbols);

  /**
  Writes header for the set text section.
  This function must be called before calling WriteSetText.
  @return the position of the set text section if succesful else returns -1
   */
  int64_t WriteSetTextHeader(int32_t number_of_set_texts);

  /**
  Writes set text to the table. Before calling this functions, you should
  call WriteSetTextHeader.
  */
  bool WriteSetText(const std::string &set_text);

  /**
  Writes set texts to GDX file.
  This function can be called once.
  @return position if successful else it returns -1
  */
  int64_t WriteSetTexts(const std::vector<std::string> &set_texts);

  /**
 Writes header for the UEL section.
 This function must be called before calling WriteUEL.
 @return the position of the UEL section if succesful else returns -1
  */
  int64_t WriteUELHeader(int32_t number_of_UELs);

  /**
   Writes UEL to the UEL table. Before calling this functions, you should
    call WriteUELHeader.
    */
  bool WriteUEL(const std::string &UEL);

  /**
  Writes UELS to GDX file.
  This function can be called once.
  @return position if successful else it returns -1
  */
  int64_t WriteUELs(const std::vector<std::string> &UELs);

  /**
  Writes header for the Acronym section.
 This function must be called before calling WriteAcronym.
 @return the position of the Acronym section if succesful else returns -1
  */
  int64_t WriteAcronymHeader(int32_t number_of_acronyms);

  /**
   Writes Acronym to the Acronym table. Before calling this functions, you
   should call WriteAcronymHeader.
    */
  bool WriteAcronym(const AcronymRaw &acronym);

  /**
 Writes Acronym to the Acronym table. Before calling this functions, you
 should call WriteAcronymHeader.
  */
  bool WriteAcronym(const std::string &name,
                    const std::string &explanatory_text, int acronym_index);

  /**
  Writes Acronyms to GDX file.
  This function can be called once.
  @return position if successful else it returns -1
  */
  int64_t WriteAcronyms(const std::vector<AcronymRaw> &acronyms);

  /**
  Writes relaxed domais.
  This function can only be called once.
  @return position if successful else it returns -1
  */
  int64_t WriteRelaxedDomains(const RelaxedDomainsRaw &relaxed_domains);

  /**
  Writes the header for the Relaxed Domain Section.
  This function must be called before calling WriteRelaxedDomainSetName
  An example call sequence:
  ```
    gdx.WriteRelaxedDomainSetsHeader(2);
    gdx.WriteRelaxedDomainSetName("i");
    gdx.WriteRelaxedDomainSetName("j");
    int domains[2] = {1, 2}; // 1 points to i, 2 points to j
    gdx.WriteRelaxedDomainSymbol(1, 2, domains);

  ```
   */
  int64_t WriteRelaxedDomainSetsHeader(int32_t number_of_domain_set_names);

  /**
  Writes the name of the used relaxed domain set.
  Before calling this function WriteRelaxedDomainSetsHeader must be called
   */
  bool WriteRelaxedDomainSetName(const std::string &domain_set_name);

  /**
  Adds relaxed domain information for the symbol with the given
  symbol_number. dim must match with the dimension of the symbol and
  domain_set_name_array must contain dim integers.
  Before calling this function, WriteRelaxedDomainSetName must be
  called for any set that will be used as relaxed domain.

  Please note that symbol number is using 1-based indexing.
  domain_set_name array also uses 1-based indexing while pointing to
  relaxed domains.
   */
  bool WriteRelaxedDomainSymbol(int symbol_number, int dim,
                                int *domain_set_name_array);

  /**
   * Set internal values for special values. Before calling this function,
   * initialize the array of special values by calling GetSpecialValues
   * first. Note: values in AVals have to be unique. Nonzero if all values
   * specified are unique, 0 otherwise.
   * Array of special values used for: Undef, NA, +Inf, -Inf, and Eps
   */
  bool SetSpecialValues(const double *special_values);

  /**
   * Retrieve the internal values for special values. Always nonzero.
   * special_values will be set to [Undef, NA, +Inf, -Inf, Eps]
   */
  bool GetSpecialValues(double *special_values);

 private:
  int64_t major_index_pos;
  Marker marker;
  bool record_left;
  int set_text_left;
  int uel_left;
  int acronym_left;
  int relaxed_domains_sets_left;
  bool relaxed_domains_symbols_left;
  double value_mapping[11];
  bool opened_for_read;
  SymbolSectionInfo symbol_section_info;
  DataSectionInfo data_section_info;
  uint8_t write_state;

  void ReadGDXHeader();
  void WriteGDXHeader();
  void ResetValueMappings();

  /**
  This function closes the symbol table and makes sure
  that the number of symbols is put in the table.
  It will be called, when we need to finish the symbol table.
  */
  void FinishSymbolHeader();

  /**
  This function closes the record and makes sure
  that write state is reset.
  */
  void FinishRecordHeader();

  /**
  This function finishes the set text section and makes sure
  that the write state is reset
  */
  void FinishSetTextHeader();

  /**
  This function finishes the UEL section and makes sure
  that the write state is reset
  */
  void FinishUELHeader();

  /**
  This function finishes the Acronym section and makes sure
  that the write state is reset
  */
  void FinishAcronymHeader();

  /**
  This function finishes the RelaxedDomainSets
  */
  void FinishRelaxedDomains();

  /**
  Before closing the GDX file, we need to update
  major index that contains position for each section
  */
  void WriteSectionPositions();

  /**
  Seeking position should reset some variable
  Use this function to seek position and reset
  those variables
  */
  void seek(int64_t pos);

  /**
  If the writing state is anything besides initial
  it calls the finish function of that state and
  sets the writing state to initial.
   */
  void FinishCurrentState();

  int32_t ReadStringSectionHeader(const char *header, int64_t pos);

  std::vector<std::string> ReadStringSection(const char *header, int64_t pos);
};
}  // namespace simplegdx
