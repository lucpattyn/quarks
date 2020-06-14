#if !defined(SORTER__INCLUDED_)
#define SORTER__INCLUDED_

#include <set>
#include <vector>

namespace QSorter {
    template <class T>
    class backwards {
        T& _obj;
    public:
        backwards(T &obj) : _obj(obj) {}
        auto begin() {return _obj.rbegin();}
        auto end() {return _obj.rend();}
    };
    
     /*struct StringValuesWithJson{
        crow::json::wvalue& _value;
        //std::string _value;
        std::string _sortby;
        
        StringValuesWithJson(crow::json::wvalue& value, std::string sortby)
            : _value(value), _sortby(sortby){
            
        }
    };
    
    struct NumberValuesWithJson{
        crow::json::wvalue& _value;
        double _sortby;
        
        NumberValuesWithJson(crow::json::wvalue& value, double sortby)
        :  _value(value), _sortby(sortby){
        }
        
    };
    
    struct StringComparer
    {
        bool operator () (const StringValuesWithJson& lhs, const StringValuesWithJson& rhs)
        {
            return (lhs._sortby.compare(rhs._sortby)) < 0 ? true : false;
        }
    };
    
    struct NumberComparer
    {
        bool operator () (const NumberValuesWithJson& lhs, const NumberValuesWithJson& rhs)
        {
            return lhs._sortby < rhs._sortby;
        }
    };*/
    
    struct JsonComparer
    {
        JsonComparer(){
            
        }
        
        JsonComparer(std::string sorter)
            :_sorter(sorter){}
        
        
        bool validate(const crow::json::rvalue& value, const crow::json::rvalue& filter){
            // sample : where: {deliveryType: {eq:”pickup”}}
            //if(filter.has("where")){
                //const crow::json::rvalue& where = filter["where"];
                // iterate all entries
                
                for (auto& obj : filter){
                    CROW_LOG_INFO << "JsonComparerValidate key : " << obj.key();
                    CROW_LOG_INFO << "JsonComparerValidate value : " << obj;
                    
                    std::string key = obj.key();
                    
                    if(!key.compare("where")){
                        auto& v = filter[key];
                        if(v.has("and")){
                            
                        }else if(v.has("or")){
                            
                        }else{
                            std::string field = v.begin()->key();
                            CROW_LOG_INFO << "JsonComparerValidate field : " << field;
                            auto& op = v[field];
                            std::string opKey = op.begin()->key();
                            
                            CROW_LOG_INFO << "JsonComparerValidate opKey : " << opKey;
                           
                            if(!opKey.compare("eq")){
                                
                                auto& check = op[opKey];
                                                                
                                if(value.has(field)){
                                    int type = -1;
                                    switch (value[field].t()) {
                                        case crow::json::type::String:{
                                            type = 0;
                                            std::string checkValue = check.s();
                                            
                                            CROW_LOG_INFO << "JsonComparerValidate check : "
                                                          << checkValue;
                                            std::string fieldValue = value[field].s();
                                            CROW_LOG_INFO << "JsonComparerValidate fieldValue : "
                                            << fieldValue;
                                            
                                            if(!fieldValue.compare(checkValue)){
                                                return true;
                                            }
                                            break;
                                        }
                                            
                                        case crow::json::type::Number:{
                                            type = 1;
                                            double checkValue = check.d();
                                            double fieldValue = value[field].d();
                                            if(fieldValue == checkValue){
                                                return true;
                                            }
                                            break;
                                        }
                                            
                                        default:;
                                    }
                                    
                                    
                                }
                            }
                        }
                        
                        return false;
                    }
                    //for (auto& nextKey : filter[key.s()]){
                    //    CROW_LOG_INFO << "JsonComparerValidate Next : " << nextKey;
                    //}
                }
                
            
            //}
            
            return false;
            
        }
        
        
        bool operator () (const crow::json::rvalue& kvLhs, const crow::json::rvalue& kvRhs)
        {
            bool ret = false;
            
            try{
                const crow::json::rvalue& lhs = kvLhs["value"];
                const crow::json::rvalue& rhs = kvRhs["value"];
                
                // 0 for string, 1 for numeric
                int typeLhs = -1;
                int typeRhs = -2;
                
                if(lhs.t() == crow::json::type::Object && lhs.has(_sorter)
                   && rhs.t() == crow::json::type::Object && rhs.has(_sorter)){
                    auto lSorter = lhs[_sorter];
                    auto rSorter = rhs[_sorter];
                    
                    switch (lSorter.t()) {
                        case crow::json::type::String:{
                            typeLhs = 0;
                            break;
                        }
                            
                        case crow::json::type::Number:{
                            typeLhs = 1;
                            break;
                        }
                        
                        default:;
                    }
                    
                    switch (rSorter.t()) {
                        case crow::json::type::String:{
                            typeRhs = 0;
                            break;
                        }
                            
                        case crow::json::type::Number:{
                            typeRhs = 1;
                            break;
                        }
                            
                        default:;
                    }
                    
                    if(typeLhs == typeRhs){
                        if(typeLhs == 1){
                           ret = compareNumeric(lSorter, rSorter);
                        }else if(typeLhs == 0){
                           ret = compareAlpha(lSorter, rSorter);
                        }
                    }
                }
                
            }catch (const std::runtime_error& error){
                CROW_LOG_INFO << "Runtime Error while sorting: " << error.what();
                //CROW_LOG_INFO << "lhs: " << crow::json::dump(lhs) << " rhs: "
                  //  << crow::json::dump(rhs) ;
                
                ret = false;
                
            };
            
            
            
            return ret;
        }
        
        bool compareAlpha (const crow::json::rvalue& lhs, const crow::json::rvalue& rhs)
        {
            std::string ls = lhs.s();
            std::string rs = rhs.s();
            
            return (ls.compare(rs) < 0) ? true : false;
            
        }
        
        bool compareNumeric(const crow::json::rvalue& lhs, const crow::json::rvalue& rhs)
        {
            int ld = lhs.d();
            int rd = rhs.d();
            
            return ld < rd;
        }
        
        std::string _sorter;
      
    };
    
    //typedef std::set<StringValuesWithJson, StringComparer> SetAlpha;
    //typedef std::set<NumberValuesWithJson, NumberComparer> SetNumeric;
    
    //typedef SetAlpha::const_iterator itAlpha;
    //typedef SetNumeric::const_iterator itNumeric;
    
}


#endif // !defined(SORTER__INCLUDED_)
