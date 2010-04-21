#ifndef __TIDYX_H__
#define __TIDYX_H__

/* tidyx.h -- C++ Wrapper for HTML Tidy Lib

  Pure syntax sugar!

  The C functions always pass a this-equivalent
  as 1st arg so, where possible, we just play along.
  For the doc proper, you can't mix C++ new/delete 
  with C malloc/free (or Tidy's over-ridden memory 
  functions).  So we just use containment there.

  For Sources and Sinks, we actually have a concrete
  struct definition to work with, so there inheritance
  is very helpful.

  Because the opaque types are pointer types, for 
  Options, Nodes and Attribute Values, the C++ 
  constructors are never actually called, instead
  we just cast the returns from the lib to C++
  pointers.  Essentially, we are copying the opaque
  data into *this;

  Copyright (c) 2002 Charles Reitzel, Arlington, MA  USA
  All Rights Reserved.

  Created 2002-07-11 by Charles Reitzel
*/

#include <tidy/tidy.h>
#include <tidy/buffio.h>

#ifdef __cplusplus

namespace Tidy
{
class Document;
class AttrVal;

/* Enums */
typedef TidyConfigCategory ConfigCategory;
typedef TidyOptionId       OptionId;
typedef TidyOptionType     OptionType;
typedef TidyTriState       TriState;
typedef TidyDoctypeModes   DoctypeModes;
typedef TidyDupAttrModes   DupAttrModes;
typedef TidyReportLevel    ReportLevel;
typedef TidyNodeType       NodeType;
typedef TidyTagId          TagId;
typedef TidyAttrId         AttrId;

/* Iterator */
typedef TidyIterator       Iterator;


/* Tidy public interface
**
** Most functions return an integer:
**
** 0    -> SUCCESS
** >0   -> WARNING
** <0   -> ERROR
** 
*/
/* I/O and Message handling interface
**
** Convert callbacks to virtual functions.
** Derive from either Source or Sink and
** provide implementations for the abstract
** I/O methods.
*/

/*****************
   Input Source
*****************/

class Source : public TidyInputSource
{
public:
  Source()
  {
    getByte    = get;
    ungetByte  = unget;
    eof        = end;
    sourceData = (ulong) this;
  }
  virtual ~Source() {}

  virtual int  GetByte() = 0;
  virtual void UngetByte( byte bv ) = 0;
  virtual Bool IsEOF() = 0;

protected:
  static int get( ulong data )
  {
    Source* source = (Source*) data;
    return ( source ? source->GetByte() : EOF );
  }
  static void unget( ulong data, byte bv )
  {
    Source* source = (Source*) data;
    if ( source )
      source->UngetByte( bv );
  }
  static Bool end( ulong data )
  {
    Source* source = (Source*) data;
    return ( source ? source->IsEOF() : yes );
  }
};


/****************
   Output Sink
****************/

class Sink : public TidyOutputSink
{
public:
  Sink()
  {
    putByte  = put;
    sinkData = (ulong) this;
  }
  virtual ~Sink() {}
  virtual void PutByte( byte bv ) = 0;

protected:
  static void put( ulong data, byte bv )
  {
    Sink* sink = (Sink*) data;
    if ( sink )
      sink->PutByte( bv );
  }
};


class Buffer : public TidyBuffer
{
public:
  Buffer()                            { Init(); }
  ~Buffer()                           { Free(); }

#ifndef SWIG
  operator ctmbstr () const { return (ctmbstr)Data(); } // May not be null
  operator tmbstr ()        { return (tmbstr)Data(); }  // terminated!
#endif

  const byte* Data() const            { return bp + next; }
  byte* Data()                        { return bp + next; }

  uint Size() const                   { return size; }
  uint Allocated() const              { return allocated; }
  Bool HasData() const                { return (Bool)( size > 0 ); }
  Bool IsEmpty() const                { return (Bool)( size == 0 ); }

  void Init()                         { tidyBufInit(this); }
  void Free()                         { tidyBufFree(this); }
  void Alloc( uint buflen )           { tidyBufAlloc(this, buflen); }
  void CheckAlloc( uint buflen, uint chunkSize = 0 )
  { tidyBufCheckAlloc(this, buflen, chunkSize);
  }

  void Attach( void* vp, uint size )  { tidyBufAttach(this, vp, size); }
  void Detach()                       { tidyBufClear(this); }

  void Clear()                        { tidyBufClear(this); }
  void Append( void* vp, uint size )  { tidyBufAppend(this, vp, size); }

  void PutByte( byte bv )             { tidyBufPutByte(this, bv); }
  int  PopByte()                      { return tidyBufPopByte(this); }

  int  GetByte()                      { return tidyBufGetByte(this); }
  Bool EndOfInput()                   { return tidyBufEndOfInput(this); }
  void UngetByte( byte bv )           { tidyBufUngetByte(this, bv); }

protected:
  TidyBuffer _buf;
};


class BufferSink : public Sink, public Buffer
{
public:
  BufferSink()                    {}
  virtual ~BufferSink()           {}

  virtual void PutByte( byte bv ) { Buffer::PutByte( bv ); }

  // Need to expose buffer methods for scripting languages
  // that do not support multiple inheritance. E.g. Java
  TidyBuffer& buf()               { return *(Buffer*)this; }
  const Buffer& buf() const       { return *(Buffer*)this; }
};

class BufferSource : public Source, public Buffer
{
public:
  BufferSource()                    {}
  virtual ~BufferSource()           {}

  virtual int  GetByte()            { return Buffer::GetByte(); }
  virtual void UngetByte( byte bv ) { Buffer::UngetByte(bv); }
  virtual Bool IsEOF()              { return Buffer::EndOfInput(); }

  // Need to expose buffer methods for scripting languages
  // that do not support multiple inheritance. E.g. Java
  Buffer& buf()                     { return *(Buffer*)this; }
  const Buffer& buf() const         { return *(Buffer*)this; }
};



class Option
{
public:
    OptionId           Id()          { return tidyOptGetId( topt() ); }
    ctmbstr            Name()        { return tidyOptGetName( topt() ); }
    OptionType         Type()        { return tidyOptGetType( topt() ); }
    Bool               IsReadOnly()  { return tidyOptIsReadOnly( topt() ); }
    ConfigCategory     Category()    { return tidyOptGetCategory( topt() ); }

    ctmbstr            Default()     { return tidyOptGetDefault( topt() ); }
    ulong              DefaultInt()  { return tidyOptGetDefaultInt( topt() ); }
    Bool               DefaultBool() { return tidyOptGetDefaultBool( topt() ); }

    Iterator           PickList()    { return tidyOptGetPickList( topt() ); }
    ctmbstr            NextPick( Iterator& pos )
    {   return tidyOptGetNextPick( topt(), &pos );
    }
protected:
    TidyOption topt()
    {  return (TidyOption) this;
    }
}; // End Option


class Node
{
public:
    /* parent / child */
    Node*   Parent()     { return (Node*) tidyGetParent( tnod() ); }
    Node*   Child()      { return (Node*) tidyGetChild( tnod() ); }

    /* siblings */
    Node*   Next()       { return (Node*) tidyGetNext( tnod() ); }
    Node*   Prev()       { return (Node*) tidyGetPrev( tnod() ); }

    /* Node info */
    NodeType Type()      { return tidyNodeGetType( tnod() ); }
    ctmbstr Name()       { return tidyNodeGetName( tnod() ); }

    /* Null for non-element nodes and all pure HTML
    ctmbstr NsLocal()    { return tidyNodeNsLocal( tnod() ); }
    ctmbstr NsPrefix()   { return tidyNodeNsPrefix( tnod() ); }
    ctmbstr NsUri()      { return tidyNodeNsUri( tnod() ); }
    */

    /* Iterate over attribute values */
    AttrVal* FirstAttr() { return (AttrVal*) tidyAttrFirst( tnod() ); }


    Bool IsText()        { return tidyNodeIsText( tnod() ); }
    Bool IsHeader()      { tidyNodeIsHeader( tnod() ); } /* h1, h2, ... */

    TagId Id()           { return tidyNodeGetId( tnod() ); }

    Bool IsHTML()        { return tidyNodeIsHTML( tnod() ); }
    Bool IsHEAD()        { return tidyNodeIsHEAD( tnod() ); }
    Bool IsTITLE()       { return tidyNodeIsTITLE( tnod() ); }
    Bool IsBASE()        { return tidyNodeIsBASE( tnod() ); }
    Bool IsMETA()        { return tidyNodeIsMETA( tnod() ); }
    Bool IsBODY()        { return tidyNodeIsBODY( tnod() ); }
    Bool IsFRAMESET()    { return tidyNodeIsFRAMESET( tnod() ); }
    Bool IsFRAME()       { return tidyNodeIsFRAME( tnod() ); }
    Bool IsIFRAME()      { return tidyNodeIsIFRAME( tnod() ); }
    Bool IsNOFRAMES()    { return tidyNodeIsNOFRAMES( tnod() ); }
    Bool IsHR()          { return tidyNodeIsHR( tnod() ); }
    Bool IsH1()          { return tidyNodeIsH1( tnod() ); }
    Bool IsH2()          { return tidyNodeIsH2( tnod() ); }
    Bool IsPRE()         { return tidyNodeIsPRE( tnod() ); }
    Bool IsLISTING()     { return tidyNodeIsLISTING( tnod() ); }
    Bool IsP()           { return tidyNodeIsP( tnod() ); }
    Bool IsUL()          { return tidyNodeIsUL( tnod() ); }
    Bool IsOL()          { return tidyNodeIsOL( tnod() ); }
    Bool IsDL()          { return tidyNodeIsDL( tnod() ); }
    Bool IsDIR()         { return tidyNodeIsDIR( tnod() ); }
    Bool IsLI()          { return tidyNodeIsLI( tnod() ); }
    Bool IsDT()          { return tidyNodeIsDT( tnod() ); }
    Bool IsDD()          { return tidyNodeIsDD( tnod() ); }
    Bool IsTABLE()       { return tidyNodeIsTABLE( tnod() ); }
    Bool IsCAPTION()     { return tidyNodeIsCAPTION( tnod() ); }
    Bool IsTD()          { return tidyNodeIsTD( tnod() ); }
    Bool IsTH()          { return tidyNodeIsTH( tnod() ); }
    Bool IsTR()          { return tidyNodeIsTR( tnod() ); }
    Bool IsCOL()         { return tidyNodeIsCOL( tnod() ); }
    Bool IsCOLGROUP()    { return tidyNodeIsCOLGROUP( tnod() ); }
    Bool IsBR()          { return tidyNodeIsBR( tnod() ); }
    Bool IsA()           { return tidyNodeIsA( tnod() ); }
    Bool IsLINK()        { return tidyNodeIsLINK( tnod() ); }
    Bool IsB()           { return tidyNodeIsB( tnod() ); }
    Bool IsI()           { return tidyNodeIsI( tnod() ); }
    Bool IsSTRONG()      { return tidyNodeIsSTRONG( tnod() ); }
    Bool IsEM()          { return tidyNodeIsEM( tnod() ); }
    Bool IsBIG()         { return tidyNodeIsBIG( tnod() ); }
    Bool IsSMALL()       { return tidyNodeIsSMALL( tnod() ); }
    Bool IsPARAM()       { return tidyNodeIsPARAM( tnod() ); }
    Bool IsOPTION()      { return tidyNodeIsOPTION( tnod() ); }
    Bool IsOPTGROUP()    { return tidyNodeIsOPTGROUP( tnod() ); }
    Bool IsIMG()         { return tidyNodeIsIMG( tnod() ); }
    Bool IsMAP()         { return tidyNodeIsMAP( tnod() ); }
    Bool IsAREA()        { return tidyNodeIsAREA( tnod() ); }
    Bool IsNOBR()        { return tidyNodeIsNOBR( tnod() ); }
    Bool IsWBR()         { return tidyNodeIsWBR( tnod() ); }
    Bool IsFONT()        { return tidyNodeIsFONT( tnod() ); }
    Bool IsLAYER()       { return tidyNodeIsLAYER( tnod() ); }
    Bool IsSPACER()      { return tidyNodeIsSPACER( tnod() ); }
    Bool IsCENTER()      { return tidyNodeIsCENTER( tnod() ); }
    Bool IsSTYLE()       { return tidyNodeIsSTYLE( tnod() ); }
    Bool IsSCRIPT()      { return tidyNodeIsSCRIPT( tnod() ); }
    Bool IsNOSCRIPT()    { return tidyNodeIsNOSCRIPT( tnod() ); }
    Bool IsFORM()        { return tidyNodeIsFORM( tnod() ); }
    Bool IsTEXTAREA()    { return tidyNodeIsTEXTAREA( tnod() ); }
    Bool IsBLOCKQUOTE()  { return tidyNodeIsBLOCKQUOTE( tnod() ); }
    Bool IsAPPLET()      { return tidyNodeIsAPPLET( tnod() ); }
    Bool IsOBJECT()      { return tidyNodeIsOBJECT( tnod() ); }
    Bool IsDIV()         { return tidyNodeIsDIV( tnod() ); }
    Bool IsSPAN()        { return tidyNodeIsSPAN( tnod() ); }
    Bool IsINPUT()       { return tidyNodeIsINPUT( tnod() ); }
    Bool IsQ()           { return tidyNodeIsQ( tnod() ); }
    Bool IsLABEL()       { return tidyNodeIsLABEL( tnod() ); }
    Bool IsH3()          { return tidyNodeIsH3( tnod() ); }
    Bool IsH4()          { return tidyNodeIsH4( tnod() ); }
    Bool IsH5()          { return tidyNodeIsH5( tnod() ); }
    Bool IsH6()          { return tidyNodeIsH6( tnod() ); }
    Bool IsADDRESS()     { return tidyNodeIsADDRESS( tnod() ); }
    Bool IsXMP()         { return tidyNodeIsXMP( tnod() ); }
    Bool IsSELECT()      { return tidyNodeIsSELECT( tnod() ); }
    Bool IsBLINK()       { return tidyNodeIsBLINK( tnod() ); }
    Bool IsMARQUEE()     { return tidyNodeIsMARQUEE( tnod() ); }
    Bool IsEMBED()       { return tidyNodeIsEMBED( tnod() ); }
    Bool IsBASEFONT()    { return tidyNodeIsBASEFONT( tnod() ); }
    Bool IsISINDEX()     { return tidyNodeIsISINDEX( tnod() ); }
    Bool IsS()           { return tidyNodeIsS( tnod() ); }
    Bool IsSTRIKE()      { return tidyNodeIsSTRIKE( tnod() ); }
    Bool IsU()           { return tidyNodeIsU( tnod() ); }
    Bool IsMENU()        { return tidyNodeIsMENU( tnod() ); }

    /* Attribute retrieval
    */
    AttrVal* GetHREF()        { return attr(tidyAttrGetHREF( tnod() )); }
    AttrVal* GetSRC()         { return attr(tidyAttrGetSRC( tnod() )); }
    AttrVal* GetID()          { return attr(tidyAttrGetID( tnod() )); }
    AttrVal* GetNAME()        { return attr(tidyAttrGetNAME( tnod() )); }
    AttrVal* GetSUMMARY()     { return attr(tidyAttrGetSUMMARY( tnod() )); }
    AttrVal* GetALT()         { return attr(tidyAttrGetALT( tnod() )); }
    AttrVal* GetLONGDESC()    { return attr(tidyAttrGetLONGDESC( tnod() )); }
    AttrVal* GetUSEMAP()      { return attr(tidyAttrGetUSEMAP( tnod() )); }
    AttrVal* GetISMAP()       { return attr(tidyAttrGetISMAP( tnod() )); }
    AttrVal* GetLANGUAGE()    { return attr(tidyAttrGetLANGUAGE( tnod() )); }
    AttrVal* GetTYPE()        { return attr(tidyAttrGetTYPE( tnod() )); }
    AttrVal* GetVALUE()       { return attr(tidyAttrGetVALUE( tnod() )); }
    AttrVal* GetCONTENT()     { return attr(tidyAttrGetCONTENT( tnod() )); }
    AttrVal* GetTITLE()       { return attr(tidyAttrGetTITLE( tnod() )); }
    AttrVal* GetXMLNS()       { return attr(tidyAttrGetXMLNS( tnod() )); }
    AttrVal* GetDATAFLD()     { return attr(tidyAttrGetDATAFLD( tnod() )); }
    AttrVal* GetWIDTH()       { return attr(tidyAttrGetWIDTH( tnod() )); }
    AttrVal* GetHEIGHT()      { return attr(tidyAttrGetHEIGHT( tnod() )); }
    AttrVal* GetFOR()         { return attr(tidyAttrGetFOR( tnod() )); }
    AttrVal* GetSELECTED()    { return attr(tidyAttrGetSELECTED( tnod() )); }
    AttrVal* GetCHECKED()     { return attr(tidyAttrGetCHECKED( tnod() )); }
    AttrVal* GetLANG()        { return attr(tidyAttrGetLANG( tnod() )); }
    AttrVal* GetTARGET()      { return attr(tidyAttrGetTARGET( tnod() )); }
    AttrVal* GetHTTP_EQUIV()  { return attr(tidyAttrGetHTTP_EQUIV(tnod())); }
    AttrVal* GetREL()         { return attr(tidyAttrGetREL( tnod() )); }
    AttrVal* GetOnMOUSEMOVE() { return attr(tidyAttrGetOnMOUSEMOVE(tnod())); }
    AttrVal* GetOnMOUSEDOWN() { return attr(tidyAttrGetOnMOUSEDOWN(tnod())); }
    AttrVal* GetOnMOUSEUP()   { return attr(tidyAttrGetOnMOUSEUP( tnod() )); }
    AttrVal* GetOnCLICK()     { return attr(tidyAttrGetOnCLICK( tnod() )); }
    AttrVal* GetOnMOUSEOVER() { return attr(tidyAttrGetOnMOUSEOVER(tnod())); }
    AttrVal* GetOnMOUSEOUT()  { return attr(tidyAttrGetOnMOUSEOUT(tnod())); }
    AttrVal* GetOnKEYDOWN()   { return attr(tidyAttrGetOnKEYDOWN( tnod() )); }
    AttrVal* GetOnKEYUP()     { return attr(tidyAttrGetOnKEYUP( tnod() )); }
    AttrVal* GetOnKEYPRESS()  { return attr(tidyAttrGetOnKEYPRESS(tnod())); }
    AttrVal* GetOnFOCUS()     { return attr(tidyAttrGetOnFOCUS( tnod() )); }
    AttrVal* GetOnBLUR()      { return attr(tidyAttrGetOnBLUR( tnod() )); }
    AttrVal* GetBGCOLOR()     { return attr(tidyAttrGetBGCOLOR( tnod() )); }
    AttrVal* GetLINK()        { return attr(tidyAttrGetLINK( tnod() )); }
    AttrVal* GetALINK()       { return attr(tidyAttrGetALINK( tnod() )); }
    AttrVal* GetVLINK()       { return attr(tidyAttrGetVLINK( tnod() )); }
    AttrVal* GetTEXT()        { return attr(tidyAttrGetTEXT( tnod() )); }
    AttrVal* GetSTYLE()       { return attr(tidyAttrGetSTYLE( tnod() )); }
    AttrVal* GetABBR()        { return attr(tidyAttrGetABBR( tnod() )); }
    AttrVal* GetCOLSPAN()     { return attr(tidyAttrGetCOLSPAN( tnod() )); }
    AttrVal* GetROWSPAN()     { return attr(tidyAttrGetROWSPAN( tnod() )); }

protected:
    AttrVal* attr( TidyAttr tattr )
    {   return (AttrVal*) tattr;
    }
    Node* node( TidyNode tnod )
    {   return (Node*) tnod;
    }
    TidyNode tnod()
    {   return (TidyNode) this;
    }
}; // End Node

/* Attribute interrogation
*/

class AttrVal
{
public:
    AttrVal* Next()        { return (AttrVal*) tidyAttrNext( tattr() ); }

    ctmbstr Name()         { return tidyAttrName( tattr() ); }
    ctmbstr Value()        { return tidyAttrValue( tattr() ); }

    /* Null for pure HTML
    ctmbstr NsLocal()      { return tidyAttrNsLocal( tattr() ); }
    ctmbstr NsPrefix()     { return tidyAttrNsPrefix( tattr() ); }
    ctmbstr NsUri()        { return tidyAttrNsUri( tattr() ); }
    */

    AttrId Id()            { return tidyAttrGetId( tattr() ); }
    Bool IsEvent()         { return tidyAttrIsEvent( tattr() ); }
    Bool IsProp()          { return tidyAttrIsProp( tattr() ); }

    Bool IsHREF()          { return tidyAttrIsHREF( tattr() ); }
    Bool IsSRC()           { return tidyAttrIsSRC( tattr() ); }
    Bool IsID()            { return tidyAttrIsID( tattr() ); }
    Bool IsNAME()          { return tidyAttrIsNAME( tattr() ); }
    Bool IsSUMMARY()       { return tidyAttrIsSUMMARY( tattr() ); }
    Bool IsALT()           { return tidyAttrIsALT( tattr() ); }
    Bool IsLONGDESC()      { return tidyAttrIsLONGDESC( tattr() ); }
    Bool IsUSEMAP()        { return tidyAttrIsUSEMAP( tattr() ); }
    Bool IsISMAP()         { return tidyAttrIsISMAP( tattr() ); }
    Bool IsLANGUAGE()      { return tidyAttrIsLANGUAGE( tattr() ); }
    Bool IsTYPE()          { return tidyAttrIsTYPE( tattr() ); }
    Bool IsVALUE()         { return tidyAttrIsVALUE( tattr() ); }
    Bool IsCONTENT()       { return tidyAttrIsCONTENT( tattr() ); }
    Bool IsTITLE()         { return tidyAttrIsTITLE( tattr() ); }
    Bool IsXMLNS()         { return tidyAttrIsXMLNS( tattr() ); }
    Bool IsDATAFLD()       { return tidyAttrIsDATAFLD( tattr() ); }
    Bool IsWIDTH()         { return tidyAttrIsWIDTH( tattr() ); }
    Bool IsHEIGHT()        { return tidyAttrIsHEIGHT( tattr() ); }
    Bool IsFOR()           { return tidyAttrIsFOR( tattr() ); }
    Bool IsSELECTED()      { return tidyAttrIsSELECTED( tattr() ); }
    Bool IsCHECKED()       { return tidyAttrIsCHECKED( tattr() ); }
    Bool IsLANG()          { return tidyAttrIsLANG( tattr() ); }
    Bool IsTARGET()        { return tidyAttrIsTARGET( tattr() ); }
    Bool IsHTTP_EQUIV()    { return tidyAttrIsHTTP_EQUIV( tattr() ); }
    Bool IsREL()           { return tidyAttrIsREL( tattr() ); }
    Bool IsOnMouseMove()   { return tidyAttrIsOnMOUSEMOVE( tattr() ); }
    Bool IsOnMouseDown()   { return tidyAttrIsOnMOUSEDOWN( tattr() ); }
    Bool IsOnMouseUp()     { return tidyAttrIsOnMOUSEUP( tattr() ); }
    Bool IsOnClick()       { return tidyAttrIsOnCLICK( tattr() ); }
    Bool IsOnMouseOver()   { return tidyAttrIsOnMOUSEOVER( tattr() ); }
    Bool IsOnMouseOut()    { return tidyAttrIsOnMOUSEOUT( tattr() ); }
    Bool IsOnKeyDown()     { return tidyAttrIsOnKEYDOWN( tattr() ); }
    Bool IsOnKeyUp()       { return tidyAttrIsOnKEYUP( tattr() ); }
    Bool IsOnKeyPress()    { return tidyAttrIsOnKEYPRESS( tattr() ); }
    Bool IsOnFOCUS()       { return tidyAttrIsOnFOCUS( tattr() ); }
    Bool IsOnBLUR()        { return tidyAttrIsOnBLUR( tattr() ); }
    Bool IsBGCOLOR()       { return tidyAttrIsBGCOLOR( tattr() ); }
    Bool IsLINK()          { return tidyAttrIsLINK( tattr() ); }
    Bool IsALINK()         { return tidyAttrIsALINK( tattr() ); }
    Bool IsVLINK()         { return tidyAttrIsVLINK( tattr() ); }
    Bool IsTEXT()          { return tidyAttrIsTEXT( tattr() ); }
    Bool IsSTYLE()         { return tidyAttrIsSTYLE( tattr() ); }
    Bool IsABBR()          { return tidyAttrIsABBR( tattr() ); }
    Bool IsCOLSPAN()       { return tidyAttrIsCOLSPAN( tattr() ); }
    Bool IsROWSPAN()       { return tidyAttrIsROWSPAN( tattr() ); }

protected:
    TidyAttr tattr()
    {   return (TidyAttr) this;
    }
}; // End AttrVal



class Document
{
public:
    Document() : _tdoc(NULL) {}
    virtual ~Document() { Release(); }

    int  Create()
    {
        Release();
        if ( _tdoc = tidyCreate() )
        {
            tidySetAppData( _tdoc, (ulong) this );
            tidySetReportFilter( _tdoc, ReportFilter );
            return 0;
        }
        return -1;
    }
    void Release()
    {
        tidyRelease( _tdoc );
        _tdoc = NULL;
    }

    /* Let application store a chunk of data w/ each Tidy instance.
    ** Useful for callbacks.
    */
    void  SetAppData( ulong data ) { tidySetAppData( _tdoc, data ); }
    ulong GetAppData()             { return tidyGetAppData( _tdoc ); }

    static ctmbstr ReleaseDate()  { return tidyReleaseDate(); }

    /* Diagnostics and Repair */
    int   Status()                { return tidyStatus( _tdoc ); }
    int   DetectedHtmlVersion()   { return tidyDetectedHtmlVersion( _tdoc ); }
    Bool  DetectedXhtml()         { return tidyDetectedXhtml( _tdoc ); }
    Bool  DetectedGenericXml()    { return tidyDetectedGenericXml( _tdoc ); }

    uint  ErrorCount()            { return tidyErrorCount( _tdoc ); }
    uint  WarningCount()          { return tidyWarningCount( _tdoc ); }
    uint  AccessWarningCount()    { return tidyAccessWarningCount( _tdoc ); }
    uint  ConfigErrorCount()      { return tidyConfigErrorCount( _tdoc ); }

    /* Get/Set configuration options
    */
    int   LoadConfig( ctmbstr configFile )
    {   return tidyLoadConfig( _tdoc, configFile );
    }
    int   LoadConfigEnc( ctmbstr configFile, ctmbstr charenc )
    {   return tidyLoadConfigEnc( _tdoc, configFile, charenc );
    }

    int   SetCharEncoding( ctmbstr encnam )
    {   return tidySetCharEncoding( _tdoc, encnam );
    }


    static OptionId  OptGetIdForName( ctmbstr optnam )
    {   return tidyOptGetIdForName( optnam );
    }

    Iterator  GetOptionList()
    {   return tidyGetOptionList( _tdoc );
    }
    Option*   GetNextOption( Iterator& pos )
    {   return (Option*) tidyGetNextOption( _tdoc, &pos );
    }

    Option*   GetOption( OptionId optId )
    {   return (Option*) tidyGetOption( _tdoc, optId );
    }
    Option*   GetOptionByName( ctmbstr optnam )
    {   return (Option*) tidyGetOptionByName( _tdoc, optnam );
    }


    ctmbstr       OptGetValue( OptionId optId )
    {   return tidyOptGetValue( _tdoc, optId );
    }
    Bool          OptSetValue( OptionId optId, ctmbstr val )
    {   return tidyOptSetValue( _tdoc, optId, val );
    }
    Bool          OptParseValue( ctmbstr optnam, ctmbstr val )
    {   return tidyOptParseValue( _tdoc, optnam, val );
    }
    uint          OptGetInt( OptionId optId )
    {   return tidyOptGetInt( _tdoc, optId );
    }
    Bool          OptSetInt( OptionId optId, uint val )
    {   return tidyOptSetInt( _tdoc, optId, val );
    }
    Bool          OptGetBool( OptionId optId )
    {   return tidyOptGetBool( _tdoc, optId );
    }
    Bool          OptSetBool( OptionId optId, Bool val )
    {   return tidyOptSetBool( _tdoc, optId, val );
    }


    Bool          OptResetToDefault( OptionId opt )
    {   return tidyOptResetToDefault( _tdoc, opt );
    }
    Bool          OptResetAllToDefault()
    {   return tidyOptResetAllToDefault( _tdoc );
    }
    /* reset to config (after document processing) */
    Bool          OptSnapshot()
    {   return tidyOptSnapshot( _tdoc );
    }
    Bool          OptDiffThanSnapshot()
    {   return tidyOptDiffThanSnapshot( _tdoc );
    }
    Bool          OptDiffThanDefault()
    {   return tidyOptDiffThanDefault( _tdoc );
    }
    Bool          OptResetToSnapshot()
    {   return tidyOptResetToSnapshot( _tdoc );
    }
    Bool          OptCopyConfig( Document& from )
    {   return tidyOptCopyConfig( _tdoc, from._tdoc );
    }
    ctmbstr       OptGetEncName( OptionId optId )
    {   return tidyOptGetEncName( _tdoc, optId );
    }
    ctmbstr       OptGetCurrPick( OptionId optId )
    {   return tidyOptGetCurrPick( _tdoc, optId );
    }
    Iterator      OptGetDeclTagList()
    {   return tidyOptGetDeclTagList( _tdoc );
    }
    ctmbstr       OptGetNextDeclTag( OptionId optId, Iterator* iter )
    {   return tidyOptGetNextDeclTag( _tdoc, optId, iter );
    }


    FILE*         SetErrorFile( ctmbstr errfilnam )
    {   return tidySetErrorFile( _tdoc, errfilnam );
    }
    int           SetErrorSink( Sink& sink )
    {   return tidySetErrorSink( _tdoc, &sink );
    }

    // Override this virtual function to filter out
    // or redirect error messages.  Return yes (true),
    // for Tidy to proceed with error output. To 
    // supress the message, return no (false).

    virtual Bool OnMessage( ReportLevel lvl, uint line, uint col, ctmbstr msg )
    {
        return yes;
    }


    /* Parse/load Functions
    **
    ** HTML/XHTML version determined from input.
    */
    int         ParseFile( ctmbstr filename )
    {   return tidyParseFile( _tdoc, filename );
    }
    int         ParseStdin()
    {   return tidyParseStdin( _tdoc );
    }
    int         ParseString( ctmbstr content )
    {   return tidyParseString( _tdoc, content );
    }
    int         ParseBuffer( Buffer& inbuf )
    {   return tidyParseBuffer( _tdoc, &inbuf );
    }
    int         ParseSource( Source& source )
    {   return tidyParseSource( _tdoc, &source );
    }


    /* Diagnostics and Repair
    */
    int         CleanAndRepair()
    {   return tidyCleanAndRepair( _tdoc );
    }
    int         RunDiagnostics()
    {   return tidyRunDiagnostics( _tdoc );
    }


    /* Document save Functions
    **
    ** If buffer is not big enough, ENOMEM will be returned and
    ** the necessary buffer size will be placed in *buflen.
    */
    int         SaveFile( ctmbstr filename )
    {   return tidySaveFile( _tdoc, filename );
    }
    int         SaveStdout()
    {   return tidySaveStdout( _tdoc );
    }
    int         SaveString( tmbstr buffer, uint* buflen )
    {   return tidySaveString( _tdoc, buffer, buflen );
    }
    int         SaveBuffer( Buffer& outbuf )
    {   return tidySaveBuffer( _tdoc, &outbuf );
    }
    int         SaveSink( Sink& sink )
    {   return tidySaveSink( _tdoc, &sink );
    }

    int         OptSaveFile( ctmbstr cfgfil )
    {   return tidyOptSaveFile( _tdoc, cfgfil );
    }
    int         OptSaveSink( Sink& sink )
    {   return tidyOptSaveSink( _tdoc, &sink );
    }

    /* Error reporting functions 
    */
    void        ErrorSummary()
    {   tidyErrorSummary( _tdoc );
    }
    void        GeneralInfo()
    {   tidyGeneralInfo( _tdoc );
    }

    /* Document tree traversal functions
    */

    Node*  GetRoot()
    {   return (Node*) tidyGetRoot( _tdoc );
    }
    Node*  GetHtml()
    {   return (Node*) tidyGetHtml( _tdoc );
    }
    Node*  GetHead()
    {   return (Node*) tidyGetHead( _tdoc );
    }
    Node*  GetBody()
    {   return (Node*) tidyGetBody( _tdoc );
    }

    Bool IsPropietary( Node* node )
    {   return tidyNodeIsProp( _tdoc, (TidyNode)node );
    }
    Bool HasText( Node* node )
    { return tidyNodeHasText( _tdoc, (TidyNode)node );
    }
    Bool GetNodeText( Node* node, Buffer& outbuf )
    { return tidyNodeGetText( _tdoc, (TidyNode)node, &outbuf );
    }


protected:
    TidyDoc _tdoc;


    /* Convert report filter callback to a virtual function.
    */
    static Bool ReportFilter( TidyDoc tdoc,
                              TidyReportLevel lvl, uint line, uint col, ctmbstr msg )
    {
        Document* doc = (Document*) tidyGetAppData( tdoc );
        if ( doc )
            return doc->OnMessage( lvl, line, col, msg );
        return no;
    }
};  // End Tidy Doc


} // End Namespace
#endif /* __cplusplus */
#endif /* __TIDYX_H__ */