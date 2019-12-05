
#include <iostream>
#include <memory>
#include <functional>
#include <array>
#include <cstdio>
#include <curl/curl.h>
#include "json.hpp"
#include <cstdint>
#include <utility>
#include <vector>
#include <sstream>
#include <ctime>
#include "sha256.hpp"
using namespace std;

typedef std::unique_ptr<CURL, std::function<void(CURL*)>> CURL_ptr;

//custom writer function datahandler
extern "C" std::size_t dataHandler(const char* buffer, std::size_t size, std::size_t nmemb, std::string* userData) {
    
    if (userData == nullptr) {
        return 0;
    }
    
    userData->append(buffer, (size * nmemb));
    return size * nmemb;
    
}

class CurlHandle{
private:
    CURL_ptr curlptr;
    constexpr static auto deleter = [](CURL* c) { //executed anytime curlptr goes out of scope.
        curl_easy_cleanup(c);
        curl_global_cleanup();
    };
    std::string data;
    
public:
    //constructor. curl_easy_init will return the CURL object
    CurlHandle(): curlptr(curl_easy_init(), deleter) {
        curl_global_init(CURL_GLOBAL_ALL); //initialize all the memory needed
        curl_easy_setopt(curlptr.get(), CURLOPT_WRITEFUNCTION, dataHandler);
        curl_easy_setopt(curlptr.get(), CURLOPT_WRITEDATA, &data);
    }
    
    //set url I want to
    void setUrl(std::string url) {
        curl_easy_setopt(curlptr.get(), CURLOPT_URL, url.c_str());
    }
    
    // fetch the url
    CURLcode fetch() {
        return curl_easy_perform(curlptr.get());
    }
    
    std::string getFetchedData() {
        return data;
    }
    
};

class Bitcoin {
    using json = nlohmann::json;
private:
    CurlHandle curlHandle;
    static constexpr const char* API_URL = "https://blockchain.info/ticker";
public:
    Bitcoin() : curlHandle({}) {
        curlHandle.setUrl(API_URL);
    }
    json fetchBitcoinData() {
        curlHandle.fetch();
        return json::parse(curlHandle.getFetchedData());
    }

};

class Block {
private:
    int _index;
    int _nonce;
    pair<string,int> _data;
    string _hash;
    time_t _time;
    string calculateHash() const {
        stringstream ss;
        ss << _index << _time << _data.first << _data.second << _nonce << prevHash;
        
        return sha256(ss.str());
    }
public:
    string prevHash;
    Block(int index, const pair<string,int> data) : _index(index), _data(data) {
        _nonce = -1;
        _time = time(NULL);
    }
    string getHash() {
        return _hash;
    }
    void mineBlock(int diff) {
        char cstr[diff + 1];
        for(int i=0; i < diff; ++i) cstr[i] = '0';
        cstr[diff] = '\0';
        string str(cstr);
        do {
            _nonce++;
            _hash = calculateHash();
        } while (_hash.substr(0, diff) != str);
        cout << "Block mined: " << _hash << endl;
    }
};

class Blockchain {
private:
    int _difficulty;
    vector<Block> chain;
    Block _getLastBlock() const {
        return chain.back();
    }
public:
    Blockchain() {
        chain.push_back(Block(0, {"Genesis Block", 0}));
        _difficulty = 4;
    }
    void addBlock(Block block) {
        block.prevHash = _getLastBlock().getHash();
        block.mineBlock(_difficulty);
        chain.push_back(block);
    }
};



int main() {
    using namespace std;
    using json = nlohmann::json;
    Blockchain blockchain = Blockchain();
    
    try {
        Bitcoin bitcoin;
        json bitcoinData = bitcoin.fetchBitcoinData();
        
        cout << "Initializing blockchain..." << endl;
        int ind = 1;
        for (auto it = bitcoinData.begin(); it != bitcoinData.end(); ++it) {
            printf("Mining block for storing the following values into the blockchain:: \t(%3s)%10d %s\n", it.key().c_str(), it.value()["last"].get<int>(),
                   it.value()["symbol"].get<string>().c_str());
            blockchain.addBlock(Block(ind++, {it.key(), it.value()["last"].get<int>()})); cout<<endl;
        }
    } catch (...) {
        cerr << "Failed to fetch bitcoin exchange rates\n";
    }
        
}
