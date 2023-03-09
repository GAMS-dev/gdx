#pragma once

namespace gdx::tests::gxfiletests {

    bool setEnvironmentVar(const std::string& name, const std::string &val);
    void unsetEnvironmentVar(const std::string& name);

    void basicTest(const std::function<void(TGXFileObj&)> &cb);
    void testRead(const std::string &filename, const std::function<void(TGXFileObj&)> &cb);
    void testWrite(const std::string &filename, const std::function<void(TGXFileObj&)> &cb);

    void writeMappedRecordsOutOfOrder(TGXFileObj &pgx);
    void domainSetGetTestSetupPrefix(TGXFileObj &pgx);
    std::string acquireGDXforModel(const std::string &model);
    void commonSetGetDomainTests(const std::vector<std::string> &domainNames, const std::vector<int> &domainIndices);
    void testReadModelGDX(const std::string &model, const std::function<void(TGXFileObj&)> &func);
    void testWithCompressConvert(bool compress, const std::string &convert);

    class StrRef {
        char *s;
    public:
        explicit StrRef(char *_s) : s(_s) {}

        StrRef &operator=(const std::string &other) {
            std::memcpy(s, other.c_str(), sizeof(char)*(other.length()+1));
            return *this;
        }

        const char *c_str() {
            return s;
        }

        [[nodiscard]] bool empty() const {
            return s[0] == '\0';
        }

        explicit operator std::string() const {
            std::string res;
            res.assign(s);
            return res;
        }

        [[nodiscard]] std::string str() const {
            std::string res;
            res.assign(s);
            return res;
        }

        bool operator==(const std::string &other) {
            return !std::strcmp(other.c_str(), s);
        }
    };

    using TgdxStrIndex = std::array<std::string, GMS_MAX_INDEX_DIM>;

    class StrIndexBuffers {
        std::array<std::array<char, GMS_SSSIZE>, GMS_MAX_INDEX_DIM> bufContents{};
        std::array<char*, GMS_MAX_INDEX_DIM> bufPtrs{};
    public:
        explicit StrIndexBuffers(const TgdxStrIndex *strIndex = nullptr) {
            for (int i{}; i < (int)bufPtrs.size(); i++) {
                bufPtrs[i] = bufContents[i].data();
                if (strIndex)
                    std::memcpy(bufPtrs[i], (*strIndex)[i].c_str(),(*strIndex)[i].length()+1);
            }
        }

        StrRef operator[](int index) {
            return StrRef{bufPtrs[index]};
        }

        char **ptrs() { return bufPtrs.data(); }
        const char** cptrs() { return (const char **)bufPtrs.data(); }

        void clear() {
            for (int i{}; i < (int)bufContents.size(); i++)
                bufContents[i].fill(0);
        }

        StrRef front() {
            return StrRef{bufPtrs[0]};
        }
    };

}