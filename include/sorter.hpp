#if !defined(SORTER__INCLUDED_)
#define SORTER__INCLUDED_

#include <set>

namespace Sorted {
    
    struct StringValuesWithJson{
        crow::json::wvalue& _value;
        //std::string _value;
        std::string _sortby;
        
        StringValuesWithJson(crow::json::wvalue& value, std::string sortby)
            : _value(value), _sortby(sortby){
            
        }
    };
    
    struct NumberValuesWithJson{
        crow::json::wvalue _value;
        double _sortby;
        
        NumberValuesWithJson(crow::json::wvalue& value, double sortby)
        :  _value(std::move(value)), _sortby(sortby){
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
    
    typedef std::set<StringValuesWithJson, StringComparer> SetAlpha;
    typedef std::set<NumberValuesWithJson, NumberComparer> SetNumeric;
    
    //typedef SetAlpha::const_iterator itAlpha;
    //typedef SetNumeric::const_iterator itNumeric;
}


#endif // !defined(SORTER__INCLUDED_)
