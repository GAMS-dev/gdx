#include "tgx.h"
#include <iostream>
#include <map>
#include <vector>

using namespace simplegdx;

void SymbolDeltaTypeArray::setDeltaType(unsigned char dimension,
                                        unsigned char type) {
  dimension -= 1;
  int major = dimension / 4;
  int minor = dimension - (4 * major);

  unsigned char clear = ~(3 << (2 * minor));

  type = type << (2 * minor);
  unsigned char *ptr = reinterpret_cast<unsigned char *>(&val_);
  unsigned char oldVal = ptr[major];
  oldVal = (oldVal & clear) | type;
  ptr[major] = oldVal;
}

unsigned char SymbolDeltaTypeArray::getDeltaType(unsigned char dimension) {
  dimension -= 1;
  int major = dimension / 4;
  int minor = dimension - (4 * major);

  unsigned char *ptr = reinterpret_cast<unsigned char *>(&val_);
  unsigned char tmp = 3;
  tmp = tmp << (2 * minor);
  tmp = tmp & ptr[major];
  tmp = tmp >> (2 * minor);
  return tmp;
}

void printArr(unsigned char *arr, int dim) {
  std::cout << "[ ";
  for (unsigned char i = 0; i < dim; i++) {
    std::cout << static_cast<int>(arr[i]);
    if (i != dim - 1) std::cout << ", ";
  }
  std::cout << " ] ";
}

void GAMSDataExchange::ResetValueMappings() {
  value_mapping[vm_valund] = simplegdx::special_value_defaults::kValUnd;
  value_mapping[vm_valna] = simplegdx::special_value_defaults::kValNa;
  value_mapping[vm_valpin] = simplegdx::special_value_defaults::kValPin;
  value_mapping[vm_valmin] = simplegdx::special_value_defaults::kValMin;
  value_mapping[vm_valeps] = simplegdx::special_value_defaults::kValEps;
  value_mapping[vm_zero] = 0;
  value_mapping[vm_one] = 1;
  value_mapping[vm_mone] = -1;
  value_mapping[vm_half] = 0.5;
  value_mapping[vm_two] = 2;
}

GAMSDataExchange::GAMSDataExchange(const std::string &fileName,
                                   const std::string &producer, bool compr)
    : ec_stream_(fileName, std::ios_base::out | std::ios_base::binary),
      compressed_(compr),
      producer_(producer),
      opened_for_read(false),
      write_state(kInit) {
  WriteGDXHeader();
  ResetValueMappings();
}

GAMSDataExchange::GAMSDataExchange(const std::string &fileName)
    : ec_stream_(fileName, std::ios_base::in | std::ios_base::binary),
      record_left(false),
      set_text_left(0),
      uel_left(0),
      acronym_left(0),
      relaxed_domains_sets_left(0),
      relaxed_domains_symbols_left(false),
      opened_for_read(true),
      symbol_section_info({0, 0}),
      write_state(kInit) {
  ReadGDXHeader();
  ResetValueMappings();
}

void GAMSDataExchange::seek(int64_t pos) {
  ec_stream_.Seek(pos);
  if (opened_for_read) {
    symbol_section_info.symbol_count = 0;
    record_left = false;
    set_text_left = 0;
    uel_left = 0;
    acronym_left = 0;
    relaxed_domains_sets_left = 0;
    relaxed_domains_symbols_left = false;
  }
}

void GAMSDataExchange::WriteGDXHeader() {
  ec_stream_ << GAMSDataExchange::kGDXHeaderNr << GAMSDataExchange::kGDXHeaderId
             << GAMSDataExchange::kGDX_Version
             << static_cast<int>(compressed_ ? 1 : 0)
             << GAMSDataExchange::kAuditTest << producer_;
  major_index_pos = ec_stream_.Tell();
  for (int i = 0; i < 10; i++) ec_stream_ << (int64_t)0;
  for (uint8_t i = 0; i < 6; i++) marker.a.positions[i] = -1;
}

void GAMSDataExchange::ReadGDXHeader() {
  unsigned char headerNr;
  std::string headerId;
  int i;
  int64_t l;
  ec_stream_ >> headerNr;
  if (headerNr != kGDXHeaderNr) throw exceptions::WrongGDXHeaderException();
  ec_stream_ >> headerId;
  ec_stream_ >> file_GDX_version;
  ec_stream_ >> i;
  compressed_ = i == 1;
  ec_stream_ >> audit_;
  ec_stream_ >> producer_;
  major_index_pos = ec_stream_.Tell();
  ec_stream_ >> i;  // compare against k_Mark_BOI

  for (int i = 0; i < 6; i++) {
    ec_stream_ >> l;
    marker.a.positions[i] = l;
  }
}

int GAMSDataExchange::ReadSymbolHeader() {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  std::string symbolsHeader;
  ec_stream_.SetCompression(compressed_);
  seek(marker.n.symbol_position);
  ec_stream_ >> symbolsHeader;
  if (symbolsHeader != "_SYMB_") throw exceptions::IncorrectMarkerException();
  ec_stream_ >> symbol_section_info.symbol_count;
  return symbol_section_info.symbol_count;
}

SymbolRaw GAMSDataExchange::ReadSymbol() {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  SymbolRaw sym;
  sym.dimension = -1;
  uint8_t dummy;
  if (symbol_section_info.symbol_count) {
    ec_stream_ >> sym.name;
    ec_stream_ >> sym.position;
    ec_stream_ >> sym.dimension;
    ec_stream_ >> sym.type;
    ec_stream_ >> sym.user_info;
    ec_stream_ >> sym.data_count;
    ec_stream_ >> sym.error_count;
    ec_stream_ >> sym.has_set_text;
    ec_stream_ >> sym.explanatory_text;
    ec_stream_ >> dummy;
    ec_stream_ >> sym.domain_controlled;

    if (sym.domain_controlled > 0) {
      for (unsigned char i = 0; i < sym.dimension; i++) {
        int dom;
        ec_stream_ >> dom;
        sym.domains.push_back(dom);
      }
    }

    ec_stream_ >> sym.comment_count;
    for (int i = 0; i < sym.comment_count; i++) {
      std::string comment;
      ec_stream_ >> comment;
      sym.comments.emplace_back(std::move(comment));
    }

    symbol_section_info.symbol_count--;
  }
  return sym;
}

bool GAMSDataExchange::ReadRecordHeader(const SymbolRaw &symbol) {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  std::string dataHeader;
  ec_stream_.SetCompression(compressed_ && symbol.dimension > 0);
  seek(symbol.position);

  const uint8_t symbol_value_dimensions[5]{1, 1, 5, 5, 0};
  data_section_info.value_dimension = symbol_value_dimensions[symbol.type];
  ec_stream_ >> dataHeader;
  if (dataHeader != "_DATA_") throw exceptions::IncorrectMarkerException();

  ec_stream_ >> data_section_info.key_dimension;

  int ignored_entry;  // this is always ignored
  ec_stream_ >> ignored_entry;

  for (unsigned char i = 0; i < data_section_info.key_dimension; i++) {
    int maxElement, minElement;
    ec_stream_ >> minElement >> maxElement;
    data_section_info.min_key[i] = minElement;
    int difference = (maxElement - minElement) + 1;
    if (difference <= 0 || difference > 65536)
      data_section_info.symbol_delta_type_arr.setDeltaType(
          i + 1, SymbolDeltaTypeArray::kInteger);
    else if (difference <= 255)
      data_section_info.symbol_delta_type_arr.setDeltaType(
          i + 1, SymbolDeltaTypeArray::kByte);
    else
      data_section_info.symbol_delta_type_arr.setDeltaType(
          i + 1, SymbolDeltaTypeArray::kWord);
  }

  record_left = true;
  return true;
}

bool GAMSDataExchange::ReadRecord(int *keyArr, double *valArr) {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  if (!record_left) return false;

  uint8_t dim = data_section_info.key_dimension;
  uint8_t value_dimension = data_section_info.value_dimension;

  SymbolDeltaTypeArray types = data_section_info.symbol_delta_type_arr;

  unsigned char delta;
  ec_stream_ >> delta;
  if (delta > dim) {
    if (delta == 255) {
      record_left = false;
      return false;
    }

    if (dim > 0) {
      keyArr[dim - 1] = data_section_info.last_key[dim - 1] + delta - dim;
      data_section_info.last_key[dim - 1] = keyArr[dim - 1];
    }
  } else {
    for (unsigned char i = delta - 1; i < dim; i++) {
      uint8_t b;
      uint16_t w;
      int32_t v;
      if (types.getDeltaType(dim) == types.kInteger) {
        ec_stream_ >> v;
      } else if (types.getDeltaType(dim) == types.kByte) {
        ec_stream_ >> b;
        v = b;
      } else {
        ec_stream_ >> w;
        v = w;
      }
      keyArr[i] = data_section_info.min_key[i] + v;
      data_section_info.last_key[i] = keyArr[i];
    }
  }

  for (unsigned char i = 0; i < value_dimension; i++) {
    unsigned char bsv;
    ec_stream_ >> bsv;
    GDXValType vType = static_cast<GDXValType>(static_cast<int>(bsv));
    if (vType == vm_normal) {
      double d;
      ec_stream_ >> d;
      // let the GMD take care of Acronyms
      valArr[i] = d;
    } else {
      valArr[i] = value_mapping[bsv];
    }
  }
  return true;
}

int32_t GAMSDataExchange::ReadStringSectionHeader(const char *header,
                                                  int64_t pos) {
  std::string start_header;
  if (compressed_) ec_stream_.SetCompression(true);
  seek(pos);
  ec_stream_ >> start_header;
  if (start_header != header) throw exceptions::IncorrectMarkerException();
  int setTextsLen;
  ec_stream_ >> setTextsLen;
  return setTextsLen;
}

std::vector<std::string> GAMSDataExchange::ReadStringSection(const char *header,
                                                             int64_t pos) {
  int setTextsLen = ReadStringSectionHeader(header, pos);
  std::vector<std::string> ret(setTextsLen);
  for (int i = 0; i < setTextsLen; i++) ec_stream_ >> ret[i];
  std::string endHeader;
  ec_stream_ >> endHeader;
  if (endHeader != header) throw exceptions::IncorrectMarkerException();
  return ret;
}

int32_t GAMSDataExchange::ReadSetTextHeader() {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  int setTextsLen =
      ReadStringSectionHeader(k_Mark_SETT, marker.n.set_text_position) - 1;
  std::string dummy;
  ec_stream_ >> dummy;
  set_text_left = setTextsLen;
  return setTextsLen;
}

bool GAMSDataExchange::ReadSetText(std::string &str) {
  if (set_text_left == 0) {
    return false;
  }
  ec_stream_ >> str;
  set_text_left--;
  return true;
}

std::vector<std::string> GAMSDataExchange::ReadSetTexts() {
  int setTextsLen = ReadSetTextHeader();
  std::vector<std::string> ret(setTextsLen);
  for (int i = 0; i < setTextsLen; i++) ec_stream_ >> ret[i];
  std::string endHeader;
  ec_stream_ >> endHeader;
  set_text_left = 0;
  if (endHeader != k_Mark_SETT) throw exceptions::IncorrectMarkerException();
  return ret;
}

int32_t GAMSDataExchange::ReadUELHeader() {
  int32_t len = ReadStringSectionHeader(k_Mark_UEL, marker.n.uel_position);
  uel_left = len;
  return len;
}

bool GAMSDataExchange::ReadUEL(std::string &str) {
  if (uel_left == 0) return false;
  ec_stream_ >> str;
  uel_left--;
  return true;
}

std::vector<std::string> GAMSDataExchange::ReadUELs() {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  return ReadStringSection(k_Mark_UEL, marker.n.uel_position);
}

int32_t GAMSDataExchange::ReadAcronymHeader() {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  if (compressed_) ec_stream_.SetCompression(true);
  seek(marker.n.acronym_position);
  std::string startHeader;
  ec_stream_ >> startHeader;
  if (startHeader != k_Mark_ACRO) throw exceptions::IncorrectMarkerException();
  int acronymCount;
  ec_stream_ >> acronymCount;
  acronym_left = acronymCount;
  return acronymCount;
}

bool GAMSDataExchange::ReadAcronym(std::string &name,
                                   std::string &explanatory_text,
                                   int &acronym_index) {
  if (acronym_left == 0) return false;
  ec_stream_ >> name >> explanatory_text >> acronym_index;
  acronym_left--;
  return true;
}

bool GAMSDataExchange::ReadAcronym(AcronymRaw &acronym) {
  if (acronym_left == 0) return false;
  ec_stream_ >> acronym.name >> acronym.explanatory_text >> acronym.map;
  acronym_left--;
  return true;
}

std::vector<AcronymRaw> GAMSDataExchange::ReadAcronyms() {
  int32_t acronymCount = ReadAcronymHeader();
  std::vector<AcronymRaw> acronyms(acronymCount);
  for (int i = 0; i < acronymCount; i++) {
    int acroMap;
    std::string acronym, explText;
    ec_stream_ >> acronym >> explText >> acroMap;
    acronyms[i].name = std::move(acronym);
    acronyms[i].explanatory_text = std::move(explText);
    acronyms[i].map = acroMap;
  }
  std::string endHeader;
  ec_stream_ >> endHeader;
  if (endHeader != k_Mark_ACRO) throw exceptions::IncorrectMarkerException();
  acronym_left = 0;
  return acronyms;
}

int32_t GAMSDataExchange::ReadRelaxedDomainsHeader() {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  int32_t num_of_sets =
      ReadStringSectionHeader(k_Mark_DOMS, marker.n.relaxed_domain_position);
  relaxed_domains_sets_left = num_of_sets;
  return num_of_sets;
}

bool GAMSDataExchange::ReadRelaxedDomainSetName(std::string &name) {
  if (relaxed_domains_sets_left == 0) return false;
  ec_stream_ >> name;
  relaxed_domains_sets_left--;
  if (relaxed_domains_sets_left == 0) {
    std::string dummy;
    ec_stream_ >> dummy;
    relaxed_domains_symbols_left = true;
  }
  return true;
}

bool GAMSDataExchange::ReadRelaxedDomainSymbol(int &symbol_number, int dim,
                                               int *domain_set_name_array) {
  if (!relaxed_domains_symbols_left) return false;
  int N;
  ec_stream_ >> N;
  if (N == -1) {
    relaxed_domains_symbols_left = false;
    return false;
  }
  symbol_number = N;
  for (unsigned char i = 0; i < dim; i++) {
    int dimValue;
    ec_stream_ >> dimValue;
    domain_set_name_array[i] = dimValue;
  }
  return true;
}

// This function requires dimension data for each symbol
RelaxedDomainsRaw GAMSDataExchange::ReadRelaxedDomains(
    const std::vector<unsigned char> &dimensions) {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  std::map<int, std::vector<int>> domainMapping;
  std::vector<std::string> domains =
      ReadStringSection(k_Mark_DOMS, marker.n.relaxed_domain_position);
  while (true) {
    int N;
    ec_stream_ >> N;
    if (N == -1) break;
    unsigned char dim = dimensions[N - 1];
    std::vector<int> symbolDom(dim);
    for (unsigned char i = 0; i < dim; i++) {
      int dimValue;
      ec_stream_ >> dimValue;
      symbolDom[i] = dimValue;
    }
    domainMapping.emplace(N, std::move(symbolDom));
  }
  std::string finishMark;
  ec_stream_ >> finishMark;
  if (finishMark != k_Mark_DOMS) throw exceptions::IncorrectMarkerException();

  return {std::move(domains), std::move(domainMapping)};
}

void GAMSDataExchange::ReadSystemInfo(int32_t *number_symbols,
                                      int32_t *number_uels) {
  if (!opened_for_read) throw exceptions::IncorrectMode();
  *number_symbols = ReadSymbolHeader();
  *number_uels = ReadStringSectionHeader(k_Mark_UEL, marker.n.uel_position);
}

int64_t GAMSDataExchange::WriteRecordHeader(simplegdx::SymbolRaw &symbol,
                                            const int32_t *minUELS,
                                            const int32_t *maxUELS) {
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (marker.n.symbol_position != -1) {
    // Data section cannot be written after the symbols section
    return -1;
  }
  if (write_state != kInit) {
    FinishCurrentState();
  }
  write_state = kWritingRecords;
  uint8_t dim = symbol.dimension;
  uint8_t symbol_type = symbol.type;

  ec_stream_.SetCompression(compressed_ && dim > 0);
  int64_t pos = ec_stream_.Tell();
  const uint8_t symbol_value_dimensions[5]{1, 1, 5, 5, 0};
  data_section_info.value_dimension = symbol_value_dimensions[symbol_type];
  data_section_info.key_dimension = dim;
  SymbolDeltaTypeArray delta_type_array;
  ec_stream_ << k_Mark_DATA << dim << -1;
  for (unsigned char i = 0; i < dim; i++) {
    data_section_info.min_key[i] = minUELS[i];

    ec_stream_ << minUELS[i] << maxUELS[i];
    int diff = maxUELS[i] - minUELS[i];
    if (diff >= 0) {
      if (diff < 256)
        delta_type_array.setDeltaType(i + 1,
                                      simplegdx::SymbolDeltaTypeArray::kByte);
      else if (diff < 65536)
        delta_type_array.setDeltaType(i + 1,
                                      simplegdx::SymbolDeltaTypeArray::kWord);
      else
        delta_type_array.setDeltaType(
            i + 1, simplegdx::SymbolDeltaTypeArray::kInteger);
    } else {
      delta_type_array.setDeltaType(i + 1,
                                    simplegdx::SymbolDeltaTypeArray::kInteger);
    }
  }
  data_section_info.symbol_delta_type_arr = delta_type_array;
  data_section_info.written = false;
  symbol.position = pos;
  symbol.data_count = 0;
  return pos;
}

bool GAMSDataExchange::WriteRecord(simplegdx::SymbolRaw &symbol,
                                   const int32_t *keyArr, double *valArr) {
  // first the key part of the record
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (write_state != kWritingRecords) {
    return false;
  }
  uint8_t dim = data_section_info.key_dimension;

  if (dim == 0) {
    ec_stream_ << (unsigned char)1;
  } else {
    uint8_t firstChangedDimension = 20;
    if (data_section_info.written) {
      for (uint8_t i = 0; i < dim; i++) {
        if (data_section_info.last_key[i] < keyArr[i]) {
          firstChangedDimension = i;
          break;
        } else if (data_section_info.last_key[i] > keyArr[i]) {
          break;
        }
      }
    } else {
      firstChangedDimension = 0;
    }
    if (firstChangedDimension == 20) return false;

    int delta = keyArr[firstChangedDimension] -
                data_section_info.last_key[firstChangedDimension];
    if (data_section_info.written && firstChangedDimension == dim - 1 &&
        delta < 255 - dim) {
      delta = dim + delta;
      ec_stream_ << (uint8_t)delta;
      data_section_info.last_key[dim - 1] = keyArr[dim - 1];
    } else {
      data_section_info.written = true;
      ec_stream_ << (unsigned char)(firstChangedDimension + 1);
      for (unsigned char i = firstChangedDimension; i < dim; i++) {
        data_section_info.last_key[i] = keyArr[i];
        if (data_section_info.symbol_delta_type_arr.getDeltaType(i + 1) ==
            SymbolDeltaTypeArray::kByte)
          ec_stream_ << (uint8_t)(keyArr[i] - data_section_info.min_key[i]);
        else if (data_section_info.symbol_delta_type_arr.getDeltaType(i + 1) ==
                 SymbolDeltaTypeArray::kWord)
          ec_stream_ << (uint16_t)(keyArr[i] - data_section_info.min_key[i]);
        else
          ec_stream_ << keyArr[i] - data_section_info.min_key[i];
      }
    }
  }

  for (unsigned char i = 0; i < data_section_info.value_dimension; i++) {
    value_mapping[10] = valArr[i];
    unsigned char valType;
    double value = valArr[i];
    for (unsigned char spv = 0; spv < 11; spv++) {
      if (value_mapping[spv] == value) {
        valType = spv;
        break;
      }
    }
    ec_stream_ << valType;
    if (valType == 10) ec_stream_ << value;
  }
  symbol.data_count++;
  return true;
}

int64_t GAMSDataExchange::WriteSymbolHeader(int32_t number_of_symbols) {
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (marker.n.symbol_position != -1) {
    return -1;
  }
  if (write_state != kInit) {
    FinishCurrentState();
  }
  ec_stream_.SetCompression(compressed_);
  write_state = kWritingSymbols;
  int64_t pos = ec_stream_.Tell();
  marker.n.symbol_position = pos;
  ec_stream_ << k_Mark_SYMB;
  ec_stream_ << number_of_symbols;
  symbol_section_info.symbol_count = 0;
  symbol_section_info.promised_symbol_count = number_of_symbols;
  return pos;
}

bool GAMSDataExchange::WriteSymbol(const SymbolRaw &symbol) {
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (write_state != kWritingSymbols ||
      symbol_section_info.promised_symbol_count ==
          symbol_section_info.symbol_count) {
    return false;
  }
  ec_stream_ << symbol.name;
  ec_stream_ << symbol.position;
  ec_stream_ << symbol.dimension;
  ec_stream_ << symbol.type;
  ec_stream_ << symbol.user_info;
  ec_stream_ << symbol.data_count;
  ec_stream_ << symbol.error_count;
  ec_stream_ << symbol.has_set_text;
  ec_stream_ << symbol.explanatory_text;
  ec_stream_ << static_cast<uint8_t>(compressed_ && (symbol.dimension > 0));
  ec_stream_ << symbol.domain_controlled;
  if (symbol.domain_controlled) {
    for (uint8_t i = 0; i < symbol.dimension; i++) {
      ec_stream_ << symbol.domains[i];
    }
  }
  ec_stream_ << symbol.comment_count;
  for (uint8_t i = 0; i < symbol.comment_count; i++) {
    ec_stream_ << symbol.comments[i];
  }
  symbol_section_info.symbol_count++;
  return true;
}

void GAMSDataExchange::FinishSymbolHeader() {
  if (symbol_section_info.promised_symbol_count >
      symbol_section_info.symbol_count) {
    // User promised to write more but wrote little
    // if not compressed we can fix it
    if (!compressed_) {
      int64_t current_position = ec_stream_.Tell();
      constexpr uint8_t length_of_symbol_marker = 7;
      seek(marker.n.symbol_position + length_of_symbol_marker);
      ec_stream_ << symbol_section_info.symbol_count;
      seek(current_position);
    } else {
      // TODO Solve this
      // when a user writes a symbol header, he/she must specify
      // the number of the symbols. We do not allow writing more
      // but user might have written less and the GDX Library
      // reading this file will expect more symbols.
    }
  }
  ec_stream_ << k_Mark_SYMB;
  write_state = kInit;
  ec_stream_.SetCompression(false);
}

int64_t GAMSDataExchange::WriteSymbols(
    const std::vector<simplegdx::SymbolRaw> &symbols) {
  int64_t sectionPosition = WriteSymbolHeader(symbols.size());
  if (sectionPosition == -1) return -1;
  for (auto &i : symbols) {
    WriteSymbol(i);
  }
  FinishSymbolHeader();
  return sectionPosition;
}

void GAMSDataExchange::WriteSectionPositions() {
  seek(major_index_pos);
  ec_stream_ << k_Mark_BOI;
  for (uint8_t i = 0; i < 6; i++) ec_stream_ << marker.a.positions[i];
}

GAMSDataExchange::~GAMSDataExchange() {
  if (!opened_for_read) {
    if (write_state != kInit) {
      FinishCurrentState();
    }

    if (marker.n.acronym_position == -1) {
      std::vector<simplegdx::AcronymRaw> empty_acronyms;
      WriteAcronyms(empty_acronyms);
    }

    if (marker.n.relaxed_domain_position == -1) {
      RelaxedDomainsRaw rdr;
      WriteRelaxedDomains(rdr);
    }

    if (marker.n.set_text_position == -1) {
      std::vector<std::string> set_texts;
      WriteSetTexts(set_texts);
    }

    if (marker.n.symbol_position == -1) {
      WriteSymbolHeader(0);
      FinishSymbolHeader();
    }

    if (marker.n.uel_position == -1) {
      std::vector<std::string> empty;
      WriteUELs(empty);
    }
    WriteSectionPositions();
  }
}

int64_t GAMSDataExchange::WriteSetTextHeader(int32_t number_of_set_texts) {
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (marker.n.set_text_position != -1) return -1;
  if (write_state != kInit) {
    FinishCurrentState();
  }
  write_state = kWritingSetTexts;
  ec_stream_.SetCompression(compressed_);
  int64_t pos = ec_stream_.Tell();
  marker.n.set_text_position = pos;
  ec_stream_ << k_Mark_SETT;
  ec_stream_ << static_cast<int32_t>(number_of_set_texts + 1);
  ec_stream_ << "";
  return pos;
}

void GAMSDataExchange::FinishSetTextHeader() {
  ec_stream_ << k_Mark_SETT;
  ec_stream_.SetCompression(false);
  write_state = kInit;
}

bool GAMSDataExchange::WriteSetText(const std::string &set_text) {
  if (write_state != kWritingSetTexts) return false;
  ec_stream_ << set_text;
  return true;
}

int64_t GAMSDataExchange::WriteSetTexts(
    const std::vector<std::string> &set_texts) {
  int64_t pos = WriteSetTextHeader(set_texts.size());
  if (pos == -1) return -1;

  for (const std::string &set_text : set_texts) {
    ec_stream_ << set_text;
  }
  FinishSetTextHeader();
  return pos;
}

int64_t GAMSDataExchange::WriteUELHeader(int32_t number_of_UELs) {
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (marker.n.uel_position != -1) return -1;

  if (write_state != kInit) {
    FinishCurrentState();
  }

  ec_stream_.SetCompression(compressed_);
  int64_t pos = ec_stream_.Tell();
  marker.n.uel_position = pos;
  ec_stream_ << k_Mark_UEL;
  ec_stream_ << static_cast<int32_t>(number_of_UELs);
  write_state = kWritingUELs;
  return pos;
}

void GAMSDataExchange::FinishUELHeader() {
  ec_stream_ << k_Mark_UEL;
  ec_stream_.SetCompression(false);
  write_state = kInit;
}

bool GAMSDataExchange::WriteUEL(const std::string &UEL) {
  if (write_state != kWritingUELs) return false;
  ec_stream_ << UEL;
  return true;
}

int64_t GAMSDataExchange::WriteUELs(const std::vector<std::string> &UELs) {
  int64_t pos = WriteUELHeader(UELs.size());
  if (pos == -1) return -1;
  for (const std::string &uel : UELs) {
    ec_stream_ << uel;
  }
  FinishUELHeader();
  return pos;
}

int64_t GAMSDataExchange::WriteAcronymHeader(int32_t number_of_acronyms) {
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (marker.n.acronym_position != -1) return -1;

  if (write_state != kInit) {
    FinishCurrentState();
  }
  write_state = kWritingAcronyms;
  ec_stream_.SetCompression(compressed_);
  int64_t pos = ec_stream_.Tell();
  marker.n.acronym_position = pos;
  ec_stream_ << k_Mark_ACRO;
  ec_stream_ << static_cast<int32_t>(number_of_acronyms);
  return pos;
}

void GAMSDataExchange::FinishAcronymHeader() {
  ec_stream_ << k_Mark_ACRO;
  ec_stream_.SetCompression(false);
  write_state = kInit;
}

bool GAMSDataExchange::WriteAcronym(const AcronymRaw &acronym) {
  if (write_state != kWritingAcronyms) return false;
  ec_stream_ << acronym.name << acronym.explanatory_text << acronym.map;
  return true;
}

bool GAMSDataExchange::WriteAcronym(const std::string &name,
                                    const std::string &explanatory_text,
                                    int acronym_index) {
  if (write_state != kWritingAcronyms) return false;
  ec_stream_ << name << explanatory_text << acronym_index;
  return true;
}

int64_t GAMSDataExchange::WriteAcronyms(
    const std::vector<AcronymRaw> &acronyms) {
  int64_t pos = WriteAcronymHeader(acronyms.size());
  if (pos == -1) return -1;
  for (const AcronymRaw &acronym : acronyms) {
    ec_stream_ << acronym.name << acronym.explanatory_text << acronym.map;
  }
  FinishAcronymHeader();
  return pos;
}

int64_t GAMSDataExchange::WriteRelaxedDomainSetsHeader(
    int32_t number_of_domain_set_names) {
  if (opened_for_read) throw exceptions::IncorrectMode();
  if (marker.n.relaxed_domain_position != -1) return -1;
  if (write_state != kInit) {
    FinishCurrentState();
  }
  ec_stream_.SetCompression(compressed_);
  int64_t pos = ec_stream_.Tell();
  marker.n.relaxed_domain_position = pos;
  ec_stream_ << k_Mark_DOMS;
  ec_stream_ << static_cast<int32_t>(number_of_domain_set_names);
  write_state = kWritingRelaxedDomainSetNames;
  return pos;
}

void GAMSDataExchange::FinishRelaxedDomains() {
  if (write_state == kWritingRelaxedDomainSetNames) {
    ec_stream_ << k_Mark_DOMS << -1 << k_Mark_DOMS;
  } else if (write_state == kWritingRelaxedDomains) {
    ec_stream_ << -1 << k_Mark_DOMS;
  }
  write_state = kInit;
}

bool GAMSDataExchange::WriteRelaxedDomainSetName(
    const std::string &domain_set_name) {
  if (write_state != kWritingRelaxedDomainSetNames) return false;
  ec_stream_ << domain_set_name;
  return true;
}

bool GAMSDataExchange::WriteRelaxedDomainSymbol(int symbol_number, int dim,
                                                int *domain_set_name_array) {
  if (write_state == kWritingRelaxedDomainSetNames) {
    ec_stream_ << k_Mark_DOMS;
    write_state = kWritingRelaxedDomains;
  }

  if (write_state != kWritingRelaxedDomains) return false;
  ec_stream_ << symbol_number;
  for (int i = 0; i < dim; i++) {
    ec_stream_ << domain_set_name_array[i];
  }

  return true;
}

int64_t GAMSDataExchange::WriteRelaxedDomains(
    const RelaxedDomainsRaw &relaxed_domains) {
  int64_t pos = WriteRelaxedDomainSetsHeader(
      relaxed_domains.used_domain_symbol_names.size());
  if (pos == -1) return -1;
  for (const std::string &used_domain :
       relaxed_domains.used_domain_symbol_names)
    ec_stream_ << used_domain;
  ec_stream_ << k_Mark_DOMS;
  for (auto &pair : relaxed_domains.symbol_index_to_domain_indexes) {
    ec_stream_ << pair.first;
    for (int i : pair.second) ec_stream_ << i;
  }
  ec_stream_ << -1 << k_Mark_DOMS;
  ec_stream_.SetCompression(false);
  return pos;
}

void GAMSDataExchange::FinishRecordHeader() {
  ec_stream_ << static_cast<uint8_t>(255);
  write_state = kInit;
  ec_stream_.SetCompression(false);
}

bool GAMSDataExchange::SetSpecialValues(const double *special_values) {
  bool duplicate = special_values[vm_valund] == special_values[vm_valna] ||
                   special_values[vm_valund] == special_values[vm_valpin] ||
                   special_values[vm_valund] == special_values[vm_valmin] ||
                   special_values[vm_valund] == special_values[vm_valeps] ||
                   special_values[vm_valna] == special_values[vm_valpin] ||
                   special_values[vm_valna] == special_values[vm_valmin] ||
                   special_values[vm_valna] == special_values[vm_valeps] ||
                   special_values[vm_valpin] == special_values[vm_valmin] ||
                   special_values[vm_valpin] == special_values[vm_valeps] ||
                   special_values[vm_valmin] == special_values[vm_valeps];
  if (duplicate) return false;
  value_mapping[vm_valund] = special_values[vm_valund];
  value_mapping[vm_valna] = special_values[vm_valna];
  value_mapping[vm_valpin] = special_values[vm_valpin];
  value_mapping[vm_valmin] = special_values[vm_valmin];
  value_mapping[vm_valeps] = special_values[vm_valeps];
  return true;
}

bool GAMSDataExchange::GetSpecialValues(double *special_values) {
  special_values[vm_valund] = value_mapping[vm_valund];
  special_values[vm_valna] = value_mapping[vm_valna];
  special_values[vm_valpin] = value_mapping[vm_valpin];
  special_values[vm_valmin] = value_mapping[vm_valmin];
  special_values[vm_valeps] = value_mapping[vm_valeps];
  return true;
}

void GAMSDataExchange::FinishCurrentState() {
  switch (write_state) {
    case kInit:
      break;
    case kWritingRecords:
      FinishRecordHeader();
      break;
    case kWritingSymbols:
      FinishSymbolHeader();
      break;
    case kWritingSetTexts:
      FinishSetTextHeader();
      break;
    case kWritingUELs:
      FinishUELHeader();
      break;
    case kWritingAcronyms:
      FinishAcronymHeader();
      break;
    case kWritingRelaxedDomainSetNames:
      FinishRelaxedDomains();
      break;
    case kWritingRelaxedDomains:
      FinishRelaxedDomains();
      break;
    default:
      throw std::invalid_argument(std::to_string(write_state));
      break;
  }
}