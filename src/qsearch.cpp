#include <qsearch.hpp>

#include <quarks.hpp>
#include <httprouting.hpp>

void QSearch::BuildHttpRoutes(void* appContext){

	crow::App<CrowMiddleware>* _app = (crow::App<CrowMiddleware>*)appContext;
	  
	crow::App<CrowMiddleware>& app = *_app;
	MMapTrie& trie = *_trie;
	
	auto route_core_fuzzy_insert_callback =
	[&trie](const crow::request& req) {
		
		std::string body = req.body;	
		auto b = req.url_params.get("body");
		if(b != nullptr) {
			body = b;
		}
		
		crow::json::wvalue out;
		auto x = crow::json::load(body);
		if (!x) {
			out["error"] = "invalid parameters";
			return out;
		}

		try {
			std::string word = x["word"].s();
			std::string tag = x["tag"].s();
			std::string meta = x["meta"].s();
 
			trie.insert(word, tag, meta);
			

		} catch (const std::runtime_error& error) {
			out["error"] = "word/tag/meta missing or not properly formatted";
		}

		out["result"] = true;

		return out;
	};


	auto route_core_fuzzy_query_callback =
	[&trie](const crow::request& req) {
		std::string body = req.body;	
		auto b = req.url_params.get("body");
		if(b != nullptr) {
			body = b;
		}
		
		crow::json::wvalue out;
		std::vector<crow::json::wvalue> jsonResults;
		
		auto x = crow::json::load(body);
		if (!x) {
			out["error"] = "invalid parameters";
			return out;
		}

		try {
			
			// Perform fuzzy search with "worl" (allowing up to 1 edit)
    		std::string searchStr = x["word"].s();
    		int maxEdits = x["maxedits"].d();
    		
    		std::string word;
    		std::string tag;
    		std::string meta;
    		
			std::vector<std::pair<std::string, WordData>> results = trie.fuzzySearch(searchStr, maxEdits);
			for (const auto& result : results) {
        		crow::json::wvalue w;
			
				word = result.first; 
				tag  = result.second.category;
                meta = result.second.userData;
                
                w["word"] = word;
                w["tag"] = tag;
                w["meta"] = meta;
    		
    			jsonResults.push_back(std::move(w));
			}	
			
		} catch (const std::runtime_error& error) {
			out["error"] = "word/maxedits missing or not properly formatted";
		}

		out["result"] = std::move(jsonResults);
		
		return out;

	};

	auto route_core_fuzzy_match_callback =
	[&trie](const crow::request& req) {
		std::string body = req.body;	
		auto b = req.url_params.get("body");
		if(b != nullptr) {
			body = b;
		}
		
		crow::json::wvalue out;
		std::vector<crow::json::wvalue> jsonResults;
		
		auto x = crow::json::load(body);
		if (!x) {
			out["error"] = "invalid parameters";
			return out;
		}

		try {
			crow::json::wvalue w;
			
			std::string searchStr = x["word"].s();
			
			std::vector<WordData> searchResults = trie.search(searchStr);
    		for (const auto& data : searchResults) {
				w["word"] = searchStr;
                w["tag"] = data.category;
                w["meta"] = data.userData;
    		
    			jsonResults.push_back(std::move(w));
			}
			

		} catch (const std::runtime_error& error) {
			out["error"] = "parameter 'word' missing";
		}

		out["result"] = std::move(jsonResults);
		
		return out;

	};
	
	auto route_core_fuzzy_prefix_callback =
	[&trie](const crow::request& req) {
		std::string body = req.body;	
		auto b = req.url_params.get("body");
		if(b != nullptr) {
			body = b;
		}
		
		crow::json::wvalue out;
		std::vector<crow::json::wvalue> jsonResults;
		
		auto x = crow::json::load(body);
		if (!x) {
			out["error"] = "invalid parameters";
			return out;
		}

		try {
			
			// Perform fuzzy search with "worl" (allowing up to 1 edit)
    		std::string searchStr = x["word"].s();
    		int maxEdits = x["maxedits"].d();
    		
			std::vector<std::pair<std::string, WordData>> results;
			if(maxEdits == -1){
				results = trie.searchByPrefix(searchStr);
				
			}else {
				results = trie.fuzzyPrefixSearch(searchStr, maxEdits);
				
			}
			
			for (const auto& result : results) {
        		crow::json::wvalue w;
			
                w["word"] = result.first;
                w["tag"] = result.second.category;
                w["meta"] = result.second.userData;
    		
    			jsonResults.push_back(std::move(w));
			}	
			
		} catch (const std::runtime_error& error) {
			out["error"] = "word/maxedits missing or not properly formatted";
		}

		out["result"] = std::move(jsonResults);
		
		return out;
	
	};
		
	auto route_core_fuzzy_substring_callback =
	[&trie](const crow::request& req) {
		std::string body = req.body;	
		auto b = req.url_params.get("body");
		if(b != nullptr) {
			body = b;
		}
		
		crow::json::wvalue out;
		std::vector<crow::json::wvalue> jsonResults;
		
		auto x = crow::json::load(body);
		if (!x) {
			out["error"] = "invalid parameters";
			return out;
		}

		try {
			
			// Perform fuzzy search with "worl" (allowing up to 1 edit)
    		std::string searchStr = x["word"].s();
    		
			std::vector<std::pair<std::string, WordData>> results;
			results = trie.substringSearch(searchStr);
			
			for (const auto& result : results) {
        		crow::json::wvalue w;
			
                w["word"] = result.first;
                w["tag"] = result.second.category;
                w["meta"] = result.second.userData;
    		
    			jsonResults.push_back(std::move(w));
			}	
			
		} catch (const std::runtime_error& error) {
			out["error"] = "word missing or not properly formatted";
		}

		out["result"] = std::move(jsonResults);
		
		return out;
	
	};
	
	CROW_ROUTE(app, "/fuzzy/insert")
	.methods("GET"_method, "POST"_method)(route_core_fuzzy_insert_callback);

	CROW_ROUTE(app, "/fuzzy/query")
	.methods("GET"_method, "POST"_method)(route_core_fuzzy_query_callback);
	
	CROW_ROUTE(app, "/fuzzy/match")
	.methods("GET"_method, "POST"_method)(route_core_fuzzy_match_callback);

	CROW_ROUTE(app, "/fuzzy/prefix")
	.methods("GET"_method, "POST"_method)(route_core_fuzzy_prefix_callback);

	CROW_ROUTE(app, "/fuzzy/substring")
	.methods("GET"_method, "POST"_method)(route_core_fuzzy_substring_callback);
}

void QSearch::TestRun() {
	
    MMapTrie& trie = *_trie; 
	// Insert words with metadata
    trie.insert("remember me, don't forget'", "poetry", "reminder");
    trie.insert("world", "noun", "planet");
    trie.insert("wordl", "noun", "globe");
    trie.insert("hello", "greeting", "salutation");
    trie.insert("hello", "greeting", "hi");

    // Perform fuzzy search with "worl" (allowing up to 1 edit)
    std::string searchStr = "remember me don't forget''";
    std::vector<std::pair<std::string, WordData>> results = trie.fuzzySearch(searchStr, 1);

    std::cout << "Fuzzy search results for '" << searchStr << "' (max 1 edit):" << std::endl;
    for (const auto& result : results) {
        std::cout << "Found: " << result.first << " -> Category: " << result.second.category
                  << ", User Data: " << result.second.userData << std::endl;
    }

	searchStr = "worl";
	results = trie.fuzzySearch(searchStr, 1);

    std::cout << "Fuzzy search results for '" << searchStr << "' (max 1 edit):" << std::endl;
    for (const auto& result : results) {
        std::cout << "Found: " << result.first << " -> Category: " << result.second.category
                  << ", User Data: " << result.second.userData << std::endl;
    }

    // Standard Search
    std::vector<WordData> searchResults = trie.search("world");
    std::cout << "Search Results:\n";
    for (const auto& data : searchResults) {
        std::cout << "Category: " << data.category << " | UserData: " << data.userData << "\n";
    }
        
    // prefix search
    trie.insert("apple", "fruit", "red");
    trie.insert("apricot", "fruit", "orange");
    trie.insert("apex", "word", "peak");
    trie.insert("banana", "fruit", "yellow");
    trie.insert("apply", "verb", "use");
    trie.insert("ape", "animal", "primate");

    // Fuzzy prefix search with max 1 edit
    std::vector<std::pair<std::string, WordData>> prefixResults = trie.fuzzyPrefixSearch("apl", 1);

    std::cout << "Words with fuzzy prefix 'apl' (max 1 edit):\n";
    std::cout << "Prefix Search Results:\n";
    for (const auto& result : prefixResults) {
        std::cout << "Found: " << result.first << " -> Category: " << result.second.category
                  << ", User Data: " << result.second.userData << std::endl;
    }
	
	
	trie.insert("hello", "greeting", "user1");
	trie.insert("world", "noun", "user2");
	trie.insert("hold", "verb", "user3");
	trie.insert("gold", "metal", "user4");

	std::vector<std::pair<std::string, WordData>> substringResults = trie.substringSearch("old");
	std::cout << "Substring Search Results:\n";
    for (const auto& result : substringResults) {
        std::cout << "Found: " << result.first << " -> Category: " << result.second.category
                  << ", User Data: " << result.second.userData << std::endl;
	}
	
	// prefix search
	trie.insert("apple", "fruit", "reddish");
    trie.insert("apricot", "fruit", "orangish");
    trie.insert("apex", "word", "cresendo");
    trie.insert("banana", "fruit", "yellowish");

    // Prefix search for "ap"
    prefixResults = trie.searchByPrefix("ap");
    std::cout << "Words with prefix 'ap':\n";
    for (const auto& result : prefixResults) {
        std::cout << "Found: " << result.first << " -> Category: " << result.second.category
                  << ", User Data: " << result.second.userData << std::endl;
	}

}

