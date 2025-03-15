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
	
	auto route_core_fuzzy_delete_callback =
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
			std::string tag = "";
			if(x.has("tag")){
				tag = x["tag"].s();
			}
			std::string meta = "";
			if(x.has("meta")){
				meta = x["meta"].s();
			}
			trie.deleteWord(word, tag, meta);
		
			out["result"] = true;	

		} catch (const std::runtime_error& error) {
			out["error"] = "Arguments not properly formatted";
			
			out["result"] = false;
		}

		return out;
	};
	
	auto route_core_fuzzy_update_callback =
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
			std::string oldWord = x["oldword"].s();
			std::string word = x["word"].s();
			std::string tag = x["tag"].s();
			std::string meta = x["meta"].s();;
			
			trie.updateWord(oldWord, word, tag, meta);
		
			out["result"] = true;	

		} catch (const std::runtime_error& error) {
			out["error"] = "oldword/word/tag/meta missing or not properly formatted";
			out["result"] = false;
		}

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
	
	CROW_ROUTE(app, "/fuzzy/delete")
	.methods("GET"_method, "POST"_method)(route_core_fuzzy_delete_callback);
	
	CROW_ROUTE(app, "/fuzzy/update")
	.methods("GET"_method, "POST"_method)(route_core_fuzzy_update_callback);
	
	
	ElasticSearch& index = *_elastic;
	
	auto route_elastic_index_callback =
	[&index](const crow::request& req) {
		
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
			std::string tenant = x["app"].s();
    		std::string indexName = x["index"].s();
    		
    		crow::json::wvalue doc = x["doc"];
    		index.indexDocument(tenant, indexName, doc);
			

		} catch (const std::runtime_error& error) {
			out["error"] = "app/index/doc missing or not properly formatted";
		}

		out["result"] = true;

		return out;
	};

	auto route_elastic_indexclear_callback =
	[&index](const crow::request& req) {
		bool ret = false;
		
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
			std::string tenant = x["app"].s();
    		std::string indexName = x["index"].s();
    		
    		ret = index.clearIndex(tenant, indexName);
			

		} catch (const std::runtime_error& error) {
			out["error"] = "app/index/doc missing or not properly formatted";
		}

		out["result"] = ret;

		return out;

	};

    //int action = 0; // 0 means search, 1 means update, -1 means delete
	
	auto route_elastic_action_callback =
	[](ElasticSearch& index, int action, const crow::request& req ) {
		
		std::cout << "elastic action callback 1" << std::endl;
		
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
		
			std::cout << "elastic action callback 2" << std::endl;
			
			std::string tenant = x["app"].s();
    		std::string indexName = x["index"].s();
    	
			std::cout << "elastic action callback 3" << std::endl;
			
			
    		// Extract conditions
    		std::vector<std::pair<std::string, std::string>> conditions;   
    		if (x.has("conditions")) {
        		crow::json::rvalue rConditions = x["conditions"];
        
        		// Ensure it's an object before iterating
        		if (rConditions.t() == crow::json::type::Object) {
	            	for (const auto& item : rConditions) {
	            		std::string value = item.s();
	                	conditions.emplace_back(item.key(), value);
	            	}
        		}
    		}

			std::cout << "elastic action callback 4" << std::endl;
			
		
			int fuzziness = 2; 
			
			bool must = false;
			if(x.has("must")){
				must = x["must"].b();
			}
			
			std::cout << "elastic action callback 5" << std::endl;
			
		
			bool ignoreCase = false;
			if(x.has("ignorecase")){
				ignoreCase = x["ignorecase"].b();
			}

			std::cout << "elastic action callback 6" << action << std::endl;
			
		
			if(action == 0){
				std::cout << "action is 0 " << std::endl;
				if(x.has("fuzziness")){
					fuzziness = x["fuzziness"].i();
				}
				auto eResults = index.searchMultiple(tenant, indexName, conditions, fuzziness, must, ignoreCase);
    
    			std::cout << "Elastic Search Results:\n";
    			for (const auto& res : eResults) {
        			std::cout << crow::json::dump(res) << std::endl;
   				}

				out["result"] = std::move(eResults);
				
			} else if(action == 1){
				fuzziness = 0;				
				if(x.has("fuzziness")){
					fuzziness = x["fuzziness"].i();
				}
			
				int updated = 0;
				if(x.has("doc")){
					crow::json::wvalue doc = x["doc"];
					updated = index.updateDocument(tenant, indexName, doc, conditions, fuzziness, must, ignoreCase);
    			}
					
    			std::cout << "Elastic update result:" << updated;
    			
				out["result"] = updated;
				
			} else if(action == -1){
				fuzziness = 0;				
				if(x.has("fuzziness")){
					fuzziness = x["fuzziness"].i();
				}
			
				int deleted = index.deleteDocument(tenant, indexName, conditions, fuzziness, must, ignoreCase);
    				
    			std::cout << "Elastic update result:" << deleted;
    			
				out["result"] = deleted;
			}
    
			
		} catch (const std::runtime_error& error) {
			out["error"] = "app/index/conditions/fuzziness/must/ignorecase missing or not properly formatted";
		}

		return out;
	};
	
	auto route_elastic_search_callback =
	[&index, &route_elastic_action_callback](const crow::request& req) {
		std::cout << "elastic search callback" << std::endl;
		auto ret = route_elastic_action_callback(index, 0, req);
		
		return ret;
	};
	
	auto route_elastic_update_callback =
	[&index, &route_elastic_action_callback](const crow::request& req) {
		return route_elastic_action_callback(index, 1, req);
	};
	
	auto route_elastic_delete_callback =
	[&index, &route_elastic_action_callback](const crow::request& req) {
		return route_elastic_action_callback(index, -1, req);
	};
	
	CROW_ROUTE(app, "/elastic/index")
	.methods("GET"_method, "POST"_method)(route_elastic_index_callback);
	
	CROW_ROUTE(app, "/elastic/index/clear")
	.methods("GET"_method, "POST"_method)(route_elastic_indexclear_callback);
	
	CROW_ROUTE(app, "/elastic/search")
	.methods("GET"_method, "POST"_method)(route_elastic_search_callback);

	CROW_ROUTE(app, "/elastic/update")
	.methods("GET"_method, "POST"_method)(route_elastic_update_callback);
	
	CROW_ROUTE(app, "/elastic/delete")
	.methods("GET"_method, "POST"_method)(route_elastic_delete_callback);
	
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
    
    trie.deleteWord("apex", "", "cresendo");
    trie.updateWord("apple", "applered", "fruit", "red");
	
    // Prefix search for "ap"
    prefixResults = trie.searchByPrefix("ap");
    std::cout << "Words with prefix 'ap':\n";
    for (const auto& result : prefixResults) {
        std::cout << "Found: " << result.first << " -> Category: " << result.second.category
                  << ", User Data: " << result.second.userData << std::endl;
	}

	ElasticSearch& index = *_elastic;
	
	std::cout << "Elastic Search Indexing ... \n";
    
	crow::json::wvalue doc;
    doc["title"] = "Elasticsearch with C++";
    doc["content"] = "This is a test query document";
    doc["timestamp"] = "2025-02-24";
    index.indexDocument("app1", "articles", doc);
    
    std::vector<std::pair<std::string, std::string>> conditions = {
        {"title", "Elesticsearch"},
        {"content", "query"}
    };
    
    std::cout << "Elastic Search Results:\n";
    auto eResults = index.searchMultiple("app1", "articles", conditions, 2, true);
    
    for (const auto& res : eResults) {
        std::cout << crow::json::dump(res) << std::endl;
    }
	
}

