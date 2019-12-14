#if !defined(SORTER__INCLUDED_)
#define SORTER__INCLUDED_

#include <set>
#include <vector>

namespace Sorter {
    
     struct StringValuesWithJson{
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
    };
    
    struct JsonComparerAlpha
    {
    	JsonComparerAlpha(std::string sorter):_sorter(sorter){}
		
        bool operator () (crow::json::rvalue& lhs, crow::json::rvalue& rhs)
        {
        	
            std::string ls = lhs[_sorter].s();
            std::string rs = rhs[_sorter].s();
            
            bool ret = (ls.compare(rs) < 0) ? true : false;
            
            return ret;
        }
        
        std::string _sorter;
    };
    
    struct JsonComparerNumeric
    {
    	JsonComparerNumeric(std::string sorter):_sorter(sorter){}
		
        bool operator () (crow::json::rvalue& lhs, crow::json::rvalue& rhs)
        {
        	
            double ld = lhs[_sorter].d();
            double rd = rhs[_sorter].d();
            
        	return ld < rd;
        }
        
        std::string _sorter;
    };
    
    
    typedef std::set<StringValuesWithJson, StringComparer> SetAlpha;
    typedef std::set<NumberValuesWithJson, NumberComparer> SetNumeric;
    
    //typedef SetAlpha::const_iterator itAlpha;
    //typedef SetNumeric::const_iterator itNumeric;
    
    typedef std::vector<StringValuesWithJson> VectorAlpha;
    typedef std::vector<NumberValuesWithJson> VectorNumeric;
    
    
}


#endif // !defined(SORTER__INCLUDED_)
