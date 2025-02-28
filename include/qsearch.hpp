#if !defined(SEARCHER__INCLUDED_)
#define SEARCHER__INCLUDED_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <mutex>
#include <unordered_set>
#include <algorithm>

#include <crow.h>

/////////////////////// Trie and Fuzzy Search /////////////////////////////////////////////

static const char* FILE_NAME = "trie_data_large.bin";
static const size_t INITIAL_SIZE = 1024 * 1024;  // 1MB initial file size

// Structure to store metadata
struct WordData {
    std::string category;
    std::string userData;

    WordData(const std::string& cat, const std::string& user)
        : category(cat), userData(user) {}
};

// Trie Node Structure (Stored in mmap)
struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    bool isEndOfWord;
    std::vector<WordData*> metadata;  // Store multiple metadata entries

    TrieNode() : isEndOfWord(false) {}
};

// Utility to get file size
static size_t qGetFileSize(const char* filename) {
    struct stat statBuf;
    if (stat(filename, &statBuf) != 0) {
        return 0;
    }
    return statBuf.st_size;
}

// Expand file size dynamically
static void qExpandFile(int fd, size_t newSize) {
    if (ftruncate(fd, newSize) != 0) {
        perror("Failed to expand file");
        exit(1);
    }
}

// Memory-Mapped Trie Class with Dynamic Growth
class MMapTrie {
private:
    int fd;
    void* mmap_data;
    size_t mmap_size;
    TrieNode* root;
    std::mutex trieMutex; // Mutex for thread safety

public:
    MMapTrie(const char* filename, size_t initialSize) {
        fd = open(filename, O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            perror("File open failed");
            exit(1);
        }

        size_t existingSize = qGetFileSize(filename);
        mmap_size = existingSize > 0 ? existingSize : initialSize;

        // Expand if needed
        if (existingSize == 0) {
            qExpandFile(fd, initialSize);
        }

        mmap_data = mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mmap_data == MAP_FAILED) {
            perror("mmap failed");
            exit(1);
        }
        root = new(mmap_data) TrieNode();
    }

    ~MMapTrie() {
        deleteTrieNode(root);
        munmap(mmap_data, mmap_size);
        close(fd);
    }

    void deleteTrieNode(TrieNode* node) {
        if (node == nullptr) return;

        // Delete the metadata entries
        for (auto* data : node->metadata) {
            delete data;
        }
        node->metadata.clear();

        // Recursively delete all child nodes
        for (auto& [ch, childNode] : node->children) {
            deleteTrieNode(childNode);
        }

        if (node != root) {
            delete node;
        }
    }

    void insert(const std::string& word, const std::string& category, const std::string& userData) {
        std::lock_guard<std::mutex> lock(trieMutex);
        TrieNode* node = root;
        for (char ch : word) {
            if (node->children.find(ch) == node->children.end()) {
                node->children[ch] = new TrieNode();
            }
            node = node->children[ch];
        }
        node->isEndOfWord = true;
        node->metadata.push_back(new WordData(category, userData)); // Store multiple metadata
    }

	/////////////////////////////// beginning of search functions ///////////////////////////////////////

    std::vector<WordData> search(const std::string& word) {
        std::lock_guard<std::mutex> lock(trieMutex);
        TrieNode* node = root;
        for (char ch : word) {
            if (node->children.find(ch) == node->children.end()) {
                return {}; // Word not found
            }
            node = node->children[ch];
        }
        if (node->isEndOfWord) {
            std::vector<WordData> results;
            for (auto* data : node->metadata) {
                results.push_back(*data);
            }
            return results;
        }
        return {}; // Word exists but is not marked as end
    }

    std::vector<std::pair<std::string, WordData>> fuzzySearch(const std::string& query, int maxEdits) {
        std::lock_guard<std::mutex> lock(trieMutex);
        std::vector<std::pair<std::string, WordData>> results;
        std::unordered_set<std::string> uniqueWords;
        fuzzySearchHelper(root, query, "", 0, 0, maxEdits, results, uniqueWords);
        return results;
    }

    void fuzzySearchHelper(TrieNode* node, const std::string& query, std::string current, int queryIndex, int edits, int maxEdits,
                           std::vector<std::pair<std::string, WordData>>& results,
                           std::unordered_set<std::string>& uniqueWords) {
        if (edits > maxEdits) return;

        if (queryIndex == query.size() && node->isEndOfWord) {
            if (uniqueWords.find(current) == uniqueWords.end()) {
                for (auto* data : node->metadata) {
                    results.push_back({current, *data});
                }
                uniqueWords.insert(current);
            }
        }

        for (auto& [ch, nextNode] : node->children) {
            int cost = (queryIndex < query.size() && query[queryIndex] == ch) ? 0 : 1;

            if (queryIndex < query.size()) {
                fuzzySearchHelper(nextNode, query, current + ch, queryIndex + 1, edits + cost, maxEdits, results, uniqueWords);
            }

            if (edits + 1 <= maxEdits) {
                fuzzySearchHelper(nextNode, query, current + ch, queryIndex, edits + 1, maxEdits, results, uniqueWords);
            }
        }

        if (edits <= maxEdits && node->isEndOfWord) {
            if (uniqueWords.find(current) == uniqueWords.end()) {
                for (auto* data : node->metadata) {
                    results.push_back({current, *data});
                }
                uniqueWords.insert(current);
            }
        }
    }

    // Recursive helper function for fuzzy prefix search
	void fuzzyPrefixHelper(TrieNode* node, const std::string& currentWord, 
                       const std::string& targetPrefix, int index, int edits, int maxEdits, 
                       std::vector<std::pair<std::string, WordData>>& results) {
	    if (!node || edits > maxEdits) return;
	
	    // If we've reached the end of the prefix and this is a valid word, add it
	    if (index >= targetPrefix.length() && node->isEndOfWord) {
	        //results.push_back({currentWord, node->metadata});
	   		for (auto* data : node->metadata) {
                results.push_back({currentWord, *data});
            }
	    }
	
	    // Check for possible character matches and mismatches
	    for (const auto& [ch, child] : node->children) {
	        if (index < targetPrefix.length()) {
	            char expectedChar = targetPrefix[index];
	            fuzzyPrefixHelper(child, currentWord + ch, targetPrefix, index + 1, 
	                              edits + (ch != expectedChar), maxEdits, results);
	        } else {
	            // If we're past the prefix, just collect all words below this node
	            fuzzyPrefixHelper(child, currentWord + ch, targetPrefix, index, edits, maxEdits, results);
	        }
	    }
	}

	// Main function to perform fuzzy prefix search
	std::vector<std::pair<std::string, WordData>> fuzzyPrefixSearch(const std::string& prefix, int maxEdits) {
    	std::lock_guard<std::mutex> lock(trieMutex);
    	std::vector<std::pair<std::string, WordData>> results;
    	fuzzyPrefixHelper(root, "", prefix, 0, 0, maxEdits, results);
    
		return results;
	}

	// substring search
	void substringSearchHelper(TrieNode* node, const std::string& substring, std::string currentWord, 
		std::vector<std::pair<std::string, WordData>>& results) {
	    
		if (node == nullptr) return;
	
	    // If this node represents a complete word, check if it contains the substring
	    if (node->isEndOfWord && currentWord.find(substring) != std::string::npos) {
	        //results.push_back({currentWord, node->metadata});
	   		for (auto* data : node->metadata) {
                results.push_back({currentWord, *data});
            }
	    }
	
	    // Recursively explore children
	    for (auto& [ch, nextNode] : node->children) {
	        substringSearchHelper(nextNode, substring, currentWord + ch, results);
	    }
	}

	std::vector<std::pair<std::string, WordData>> substringSearch(const std::string& substring) {
    	
		std::vector<std::pair<std::string, WordData>> results;
        	// Start searching from the root of the Trie
    	substringSearchHelper(root, substring, "", results);
    
   		return results;
	}

	// Recursive helper function to collect words from a given node
	void collectWordsFromNode(TrieNode* node, const std::string& prefix, 
                          std::vector<std::pair<std::string, WordData>>& results) {
    	if (node == nullptr) return;

    	if (node->isEndOfWord) {
        	//results.push_back({prefix, node->metadata});
        	for (auto* data : node->metadata) {
                results.push_back({prefix, *data});
            }
    	}

    	for (const auto& [ch, child] : node->children) {
        	collectWordsFromNode(child, prefix + ch, results);
    	}
	}	

	// Main function to search words by prefix
	std::vector<std::pair<std::string, WordData>> searchByPrefix(const std::string& prefix) {
    	std::lock_guard<std::mutex> lock(trieMutex);
    	TrieNode* node = root;
    	std::vector<std::pair<std::string, WordData>> results;
    	// Traverse to the last node of the given prefix
    	for (char ch : prefix) {
        	if (node->children.find(ch) == node->children.end()) {
            	return results;  // Prefix not found, return empty result
        	}
        	node = node->children[ch];
    	}

    	// Collect all words from this node
    	collectWordsFromNode(node, prefix, results);
    	return results;
	}

	/////////////////////////////// end of search functions ///////////////////////////////////////
	
	void deleteWord(const std::string& word, const std::string& category = "", const std::string& userData = "") {
	    std::lock_guard<std::mutex> lock(trieMutex);
	    TrieNode* node = root;
	    std::vector<TrieNode*> nodesToCheck;
	
	    // Traverse the trie for the given word.
	    for (char ch : word) {
	        if (node->children.find(ch) == node->children.end()) {
	            return; // Word not found
	        }
	        node = node->children[ch];
	        nodesToCheck.push_back(node); // Keep track of the nodes visited.
	    }
	
	    // If the word ends here and has metadata, proceed to remove it.
	    if (node->isEndOfWord) {
	        // Remove metadata if category/userData matches
	        if (category.empty() && userData.empty()) {
	            node->metadata.clear();
	        } else {
	            node->metadata.erase(std::remove_if(node->metadata.begin(), node->metadata.end(),
	                [&](WordData* data) {
	                    return (category.empty() || data->category == category) &&
	                           (userData.empty() || data->userData == userData);
	                }), node->metadata.end());
	        }
	
	        // If metadata is empty after removal, mark it as not the end of a word
	        if (node->metadata.empty()) {
	            node->isEndOfWord = false;
	        }
	    }
	
	    // Now we need to clean up the trie if no children exist and if this node is not an end of any other word
	    for (int i = nodesToCheck.size() - 1; i >= 0; --i) {
	        node = nodesToCheck[i];
	
	        // If the node has no children and it's not the end of any other word, delete it
	        if (node->children.empty() && !node->isEndOfWord) {
	            // If it's not the root node, remove it from the parent's children map.
	            if (i > 0) {
	                TrieNode* parentNode = nodesToCheck[i - 1];
	                parentNode->children.erase(word[i - 1]);
	            }
	        } else {
	            break; // Stop if we encounter a node that is still part of another word.
	        }
	    }
	}	


    void updateWord(const std::string& oldWord, const std::string& newWord, const std::string& category = "", const std::string& userData = "") {
        deleteWord(oldWord, category, userData);
		insert(newWord, category, userData);
    }


	void expandIfNeeded() {
        std::lock_guard<std::mutex> lock(trieMutex);
        size_t currentSize = qGetFileSize(FILE_NAME);
        if (currentSize >= mmap_size - 1024) {
            size_t newSize = mmap_size * 2;
            munmap(mmap_data, mmap_size);
            qExpandFile(fd, newSize);
            mmap_data = mmap(nullptr, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (mmap_data == MAP_FAILED) {
                perror("mmap failed after expansion");
                exit(1);
            }
            mmap_size = newSize;
        }
    }

};


/////////////////// end of Trie and Fuzzy Search ///////////////////////

//////////////////// Elastic Search ////////////////////////////////////


class ElasticSearch {
private:
    std::string filename;
    int fd;
    void* mmap_ptr;
    size_t file_size;
    crow::json::wvalue index;

    void loadIndex() {
        fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        file_size = lseek(fd, 0, SEEK_END);
        if (file_size == 0) {
            ftruncate(fd, 4096);
            file_size = 4096;
        }

        mmap_ptr = mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mmap_ptr == MAP_FAILED) {
            perror("Error mapping file");
            close(fd);
            exit(EXIT_FAILURE);
        }

        std::string data(static_cast<char*>(mmap_ptr), file_size);
        if (!data.empty() && data[0] != '\0') {
            index = crow::json::load(data);
        }
    }

    void persistIndex() {
        std::string json_data = crow::json::dump(index);//index.dump();
        if (json_data.size() > file_size) {
            size_t new_size = json_data.size() * 2;
            if (ftruncate(fd, new_size) == -1) {
                perror("Error resizing file");
                return;
            }
            munmap(mmap_ptr, file_size);
            mmap_ptr = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (mmap_ptr == MAP_FAILED) {
                perror("Error remapping file");
                close(fd);
                exit(EXIT_FAILURE);
            }
            file_size = new_size;
        }
        std::memcpy(mmap_ptr, json_data.c_str(), json_data.size());
    }

    static int levenshteinDistance(const std::string& s1, const std::string& s2) {
        int len1 = s1.size(), len2 = s2.size();
        std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1, 0));

        for (int i = 0; i <= len1; i++) dp[i][0] = i;
        for (int j = 0; j <= len2; j++) dp[0][j] = j;

        for (int i = 1; i <= len1; i++) {
            for (int j = 1; j <= len2; j++) {
                if (s1[i - 1] == s2[j - 1]) {
                    dp[i][j] = dp[i - 1][j - 1];
                } else {
                    dp[i][j] = std::min({ dp[i - 1][j - 1], dp[i - 1][j], dp[i][j - 1] }) + 1;
                }
            }
        }
        return dp[len1][len2];
    }
	
	// Tokenize function to split a sentence into words
	std::vector<std::string> tokenize(const std::string& s) {
	    std::vector<std::string> tokens;
	    std::stringstream ss(s);
	    std::string token;
	    while (ss >> token) {
	        tokens.push_back(token);
	    }
	    return tokens;
	}

	// Check if the second string is a nearly matched substring of the first string
	bool isNearlyMatched(const std::string& str1, const std::string& str2, int maxDistance) {
    	std::vector<std::string> tokens1 = tokenize(str1);  // Tokenize first string
    	std::vector<std::string> tokens2 = tokenize(str2);  // Tokenize second string

    	int i = 0, j = 0;
    	int len1 = tokens1.size(), len2 = tokens2.size();

	    // Traverse tokens1 and check if tokens2 is a nearly matching substring
	    while (i < len1 && j < len2) {
	        // Compare the current token from tokens1 with the current token from tokens2
	        int dist = levenshteinDistance(tokens1[i], tokens2[j]);
	
	        // If the distance is within the allowed threshold, consider it a match
	        if (dist <= maxDistance) {
	            j++;  // Move to the next token in tokens2
	        }
	        i++;  // Always move to the next token in tokens1
	    }
	
	    // Return true if all tokens from tokens2 were found with acceptable distance
	    return j == len2;
}
	

public:
    ElasticSearch(const std::string& file) : filename(file), fd(-1), mmap_ptr(nullptr), file_size(0) {
        loadIndex();
    }

    void indexDocument(const std::string& tenant_id, const std::string& indexName, crow::json::wvalue& doc) {
        index[tenant_id][indexName] = std::move(doc);
        	
		persistIndex();
    }

    void updateDocument(const std::string& tenant_id, const std::string& indexName, crow::json::wvalue& new_doc) {
        if (index[tenant_id].count(indexName)) {
            index[tenant_id][indexName] = std::move(new_doc);
            persistIndex();
        }
    }

    void deleteDocument(const std::string& tenant_id, const std::string& indexName) {
        if (index[tenant_id].count(indexName)) {
            crow::json::wvalue newTenant;
			for (const auto& key : index[tenant_id].keys()) {
	    		if (key != indexName) {
	        		newTenant[key] = std::move(index[tenant_id][key]);
	    		}
			}
			index[tenant_id] = std::move(newTenant);
			
			persistIndex();
        }
    }

    std::vector<crow::json::wvalue> searchMultiple(const std::string& tenant_id, const std::string& indexName, const std::vector<std::pair<std::string, std::string>>& conditions, int fuzziness = 2, bool must = true) {
        std::vector<crow::json::wvalue> results;

		if(index.count(tenant_id) != true) 
			return results;
		
		if(index[tenant_id].count(indexName) != true)
			return results;	
			
		//for (const auto& key : index[tenant_id][indexName].keys()){
            auto doc = crow::json::load(crow::json::dump(index[tenant_id][indexName]));
			int match_count = 0;
            for (const auto& [key, query] : conditions) {
                if (doc.has(key)) {
              		std::string value = doc[key].s();
                    if (isNearlyMatched(value, query, fuzziness)) {
                        match_count++;
                    }
                }
            }
            if ((must && match_count == conditions.size()) || (!must && match_count > 0)) {
				results.push_back(std::move(doc));
            }
        //}

        return results;
    }

    ~ElasticSearch() {
        if (mmap_ptr != MAP_FAILED && mmap_ptr != nullptr) {
            msync(mmap_ptr, file_size, MS_SYNC);
            munmap(mmap_ptr, file_size);
        }
        if (fd != -1) {
            close(fd);
        }
    }
};

///////////////////////// end of Elastic Search ////////////////////////////////////


class QSearch {
public:
	QSearch(){
		try{
			_trie = new MMapTrie(FILE_NAME, INITIAL_SIZE);
			_elastic = new ElasticSearch("elastic.dat");
		}
		catch (const std::exception& e) {
        	std::cerr << "Error: " << e.what() << std::endl;
    	}
		
	}
	
	void cleanup(){
		delete _trie;
		delete _elastic;
	}
	
	~QSearch(){
		
	}
	
	void BuildHttpRoutes(void* appContext);
	
	void TestRun();
	
private:
	MMapTrie* _trie;
	ElasticSearch* _elastic;
	
};


#endif // !defined(SEARCHER__INCLUDED_)
