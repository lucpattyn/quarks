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


/////////////////// End of Trie and Fuzzy Search ///////////////////////

class QSearch {
public:
	QSearch(){
		_trie = new MMapTrie(FILE_NAME, INITIAL_SIZE);
	}
	
	void cleanup(){
		delete _trie;
	}
	
	~QSearch(){
		
	}
	
	void BuildHttpRoutes(void* appContext);
	
	/*void insert(const std::string& word, const std::string& category, const std::string& userData) {
		_trie->insert(word, category, userData);
	}
	
	// regular search
	std::vector<WordData> search(const std::string& word){
		return _trie->search(word);
	}
	
	// fuzzy search
	std::vector<std::pair<std::string, WordData>> fuzzySearch(const std::string& query, int maxEdits) {
		return _trie->fuzzySearch(query, maxEdits);
	}*/
	
	void TestRun();
	
private:
	MMapTrie* _trie;
	
};


#endif // !defined(SEARCHER__INCLUDED_)
