/////////////////////////////////////////////////////////////////////////////
// GridCtrl.h : header file
//
// MFC Grid Control - main header
//
// Written by Chris Maunder <chris@codeproject.com>
// Copyright (c) 1998-2005. All Rights Reserved.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name and all copyright 
// notices remains intact. 
//
// An email letting me know how you are using it would be nice as well. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
// For use with CGridCtrl v2.20+
//
//////////////////////////////////////////////////////////////////////

#if !defined(MATRIX__INCLUDED_)
#define MATRIX__INCLUDED_

#include  <memory> // We need to include this for shared_ptr
#include <string>
#include <map>
#include <vector>

#include <crow.h>

namespace Quarks {
    
    struct Matrix{
        
        static Matrix _Instance;
        
        enum CellType{
            Undefined = 0,
            String,
            Boolean,
            Int,
            Real,
            DateTime,
            Json
        };
        
        struct CellValue{
            std::string s;
            bool b;
            int i;
            double r;
            unsigned long d;
            crow::json::rvalue j;
            
            CellValue(){
                s = "";
                b = false;
                i = 0;
                r = 0.0;
                d = 0;
            }
        };
        
        struct Cell{
            virtual int compare(CellValue& v){
                return v.i;
                
            }
            
            virtual ~Cell(){};
        };
        
        struct CellText : public Cell{
            virtual int compare(CellValue& v){
                return value.compare(v.s);
            }
            
            virtual ~CellText(){
                
            }
            
            std::string value;
            
            
        };
        
        struct CellBoolean : public Cell{
            virtual int compare(CellValue& v){
                return (value == v.b)? 0 : -1;
            }
            
            virtual ~CellBoolean(){
                
            }
            
            bool value;
        };
        
        struct CellInt : public Cell{
            virtual int compare(CellValue& v){
                return (value - v.i);
            }
            
            virtual ~CellInt(){
                
            }
            
            int value;
        };
        
        struct CellReal : public Cell{
            virtual int compare(CellValue& v){
                return ((value - v.r) < 0.0) ? -1 : 1;
            }
            
            virtual ~CellReal(){
                
                
            }
            double value;
        };
        
        struct CellDateTime : public Cell{
            virtual int compare(CellValue& v){
                return (value - v.d);
            }
            
            virtual ~CellDateTime(){
                
            }
            
            unsigned long value;
        };
        
        struct CellJson : public Cell{
            virtual int compare(CellValue& v){
                return  v.i;
            }
            
            virtual ~CellJson(){
                
            }
            
            crow::json::rvalue value;
        };
        
        std::shared_ptr<Cell> create(const CellType type)
        {
            switch((int)type){
                case String:
                    return std::make_shared<CellText>();
                
                case Boolean:
                    return std::make_shared<CellBoolean>();
                
                case Int:
                    return std::make_shared<CellInt>();
                
                case Real:
                    return std::make_shared<CellReal>();
                
                case DateTime:
                    return std::make_shared<CellDateTime>();
                
                case Json:
                    return std::make_shared<CellJson>();
                    
                default:
                    return std::make_shared<Cell>();
                
            }
            
            return std::make_shared<Cell>();
           
        }
        
        void addCell(std::string key, std::shared_ptr<Cell> cell){
            _matrixData[key].push_back(cell);
            
        }
        
        void replaceCell(std::string key, int col, std::shared_ptr<Cell> cell){
            _matrixData[key][col].reset();
            
            _matrixData[key][col] = cell;
        }
        
        typedef std::vector<std::shared_ptr<Cell>> RowData;
        typedef std::map<std::string, RowData> MatrixData;
        
        MatrixData _matrixData;
        
    };

}

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(MATRIX__INCLUDED_)
