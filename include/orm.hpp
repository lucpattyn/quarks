#include <functional> // for std::logical_and, std::logical_or

struct ORM {
    // Validate method for applying filters
    bool validate(const crow::json::rvalue& jsonValue, const crow::json::rvalue& filterBy) {
        try {
        	//CROW_LOG_INFO << "ORMValidate jsonValue : " << jsonValue;
        	//CROW_LOG_INFO << "ORMValidate filterBy : " << filterBy;
            
			if (filterBy.has("where")) {
			    const crow::json::rvalue& whereClause = filterBy["where"];
                //CROW_LOG_INFO << "ORMValidate whereClause : " << whereClause;
                
                if (whereClause.has("and")) {
                    return processLogicalOperator(jsonValue, whereClause["and"], std::logical_and<>());
                }
                
                if (whereClause.has("or")) {
                    return processLogicalOperator(jsonValue, whereClause["or"], std::logical_or<>());
                }
                
                return applyFilter(jsonValue, whereClause);
            }
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Error in ORM::validate: " << e.what();
            return false;
        }
        
		return false;
    }

private:
    // Apply individual filters like eq and eq_any
    bool applyFilter(const crow::json::rvalue& jsonValue, const crow::json::rvalue& whereClause) {
    	
		//CROW_LOG_INFO << "ORMApplyFilter jsonValue : " << jsonValue;
        //CROW_LOG_INFO << "ORMApplyFilter whereClause : " << whereClause;
        
		for (auto it = whereClause.begin(); it != whereClause.end(); ++it) {
            auto& key = it->key();
			//CROW_LOG_INFO << "ORMValidate key : " << key;
            
			auto& condition = *it;
            //CROW_LOG_INFO << "ORMValidate value : " << condition;
			
			if (condition.has("eq")) {
                if (checkEq(jsonValue, key, condition["eq"])) {
                    return true;
                }
            } else if (condition.has("eq_any")) {
                if (checkEqAny(jsonValue, key, condition["eq_any"])) {
                    return true;
                }
            } else if (condition.has("neq")) {
                if (checkNotEq(jsonValue, key, condition["neq"])) {
                    return true;
                }
            } else if (condition.has("not_in")) {
                if (checkNotIn(jsonValue, key, condition["not_in"])) {
                    return true;
                }
            }
        }
        
        return false;
    }

    // Helper to process logical operators ('and'/'or')
    template <typename LogicalOp>
    bool processLogicalOperator(const crow::json::rvalue& jsonValue, const crow::json::rvalue& conditions, LogicalOp op) {
        bool result = false;
        bool firstCondition = true;

        for (auto& condition : conditions) {
            bool conditionResult = applyFilter(jsonValue, condition);
            
            if (firstCondition) {
                result = conditionResult;
                firstCondition = false;
            } else {
                result = op(result, conditionResult);
            }
        }
        return result;
    }

    // Centralized comparison helper with exception handling
    template <typename T>
    bool compareValues(const crow::json::rvalue& jsonValue, const std::string& key, const T& targetValue) {
        try {
            if (jsonValue.has(key)) {
                const auto& fieldValue = jsonValue[key];
                switch (fieldValue.t()) {
                    case crow::json::type::String: {
                        return compareString(fieldValue.s(), targetValue.s());
                    }
                    case crow::json::type::Number: {
                        return compareNumber(fieldValue.d(), targetValue.d());
                    }
                    case crow::json::type::True: {
                        return compareBool(fieldValue.b(), targetValue.b());
                    }
                    case crow::json::type::False: {
                        return compareBool(fieldValue.b(), targetValue.b());
                    }
                    default:
                        return false;
                }
            }
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Error in compareValues for key: " << key << " - " << e.what();
            return false;
        }
        return false;
    }

    // Specialized comparison for strings with exception handling
    bool compareString(const std::string& fieldValue, const std::string& checkValue) {
        try {
            return fieldValue == checkValue;
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Error in compareString: " << e.what();
            return false;
        }
    }

    // Specialized comparison for numbers with exception handling
    bool compareNumber(double fieldValue, double checkValue) {
        try {
            return fieldValue == checkValue;
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Error in compareNumber: " << e.what();
            return false;
        }
    }

    // Specialized comparison for booleans with exception handling
    bool compareBool(bool fieldValue, bool checkValue) {
        try {
            return fieldValue == checkValue;
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Error in compareBool: " << e.what();
            return false;
        }
    }

    // 'eq' comparison with type safety and exception handling
    bool checkEq(const crow::json::rvalue& jsonValue, const std::string& key, const crow::json::rvalue& targetValue) {
        return compareValues(jsonValue, key, targetValue);
    }

    // 'eq_any' comparison with type safety and exception handling
    bool checkEqAny(const crow::json::rvalue& jsonValue, const std::string& key, const crow::json::rvalue& targetValues) {
        try {
            if (jsonValue.has(key)) {
                for (const auto& check : targetValues) {
                    if (compareValues(jsonValue, key, check)) {
                        return true;
                    }
                }
            }
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Error in checkEqAny: " << e.what();
        }
        return false;
    }
 // 'neq' comparison with type safety and exception handling
    bool checkNotEq(const crow::json::rvalue& jsonValue, const std::string& key, const crow::json::rvalue& targetValue) {
        return !compareValues(jsonValue, key, targetValue);
    }

    // 'not_in' comparison with type safety and exception handling
    bool checkNotIn(const crow::json::rvalue& jsonValue, const std::string& key, const crow::json::rvalue& targetValues) {
        try {
            if (jsonValue.has(key)) {
                for (const auto& check : targetValues) {
                    if (compareValues(jsonValue, key, check)) {
                        return false;
                    }
                }
            }
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Error in checkNotIn: " << e.what();
        }
        return true;
    }

};

