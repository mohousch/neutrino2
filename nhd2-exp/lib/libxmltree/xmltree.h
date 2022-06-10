/* xmltree.h
 *
 * XMLTree API header
 *
 * $Id: xmltree.h,v 1.3 2009/11/22 15:36:38 rhabarber1848 Exp $
 *
 */

#ifndef __XMLTREE_H
#define __XMLTREE_H

#include "xmlparse.h"

class XMLAttribute
{
  private:
    char         *name, *value;
    XMLAttribute *next, *prev;

  public:
    XMLAttribute();
    XMLAttribute(char *nam, char *val);
    XMLAttribute(XMLAttribute *prv, char *nam, char *val);
    XMLAttribute(XMLAttribute *prv, char *nam, char *val, XMLAttribute *nxt);
    ~XMLAttribute();

    char         *GetName() { return name; }
    char         *GetValue() { return value; }
    XMLAttribute *GetNext() { return next; }
    XMLAttribute *GetPrevious() { return prev; }

    void          SetName(char *nam);
    void          SetValue(char *val);
    void          SetNext(XMLAttribute *nxt);
    void          SetPrevious(XMLAttribute *prv);
};

class XMLTreeNode
{
  public:
    // matching mode: case insensitive or case sensitive?

    enum matchmode
    {
      MATCH_CASE   = 0,
      MATCH_NOCASE = 1
    };

  private:
    XMLAttribute *attributes;
    XMLTreeNode  *next;
    XMLTreeNode  *child;
    XMLTreeNode  *parent;

    char         *type;
    char         *data;
    unsigned int  dataSize;
    unsigned int  pdataOff;

    matchmode     mmode;

  public:
    XMLTreeNode(XMLTreeNode *prnt);
    XMLTreeNode(XMLTreeNode *prnt, char *typ);
    XMLTreeNode(XMLTreeNode *prnt, char *typ, char *dta, unsigned int dtaSize);
    XMLTreeNode(XMLTreeNode *prnt, char *typ, char *dta, unsigned int dtaSize, XMLTreeNode *cld);
    XMLTreeNode(XMLTreeNode *prnt, char *typ, char *dta, unsigned int dtaSize, XMLTreeNode *cld, XMLTreeNode *nxt);
    ~XMLTreeNode();

    // add modes: add the node as child or neighbour?

    enum addmode
    {
      ADD_NEIGHBOUR = 0,
      ADD_CHILD     = 1
    };

    XMLTreeNode  *GetNext() const { return next; }
    XMLTreeNode  *GetChild() const { return child; }
    XMLTreeNode  *GetParent() const { return parent; };

    XMLAttribute *GetAttributes() const { return attributes; }
    XMLAttribute *GetAttribute(const char *name) const;
    char         *GetAttributeValue(const char *name) const;

    matchmode     GetMatchingMode() const { return mmode; }

    char         *GetType() const { return type; }
    char         *GetData() const { return data; }
    unsigned int  GetDataSize() const { return dataSize; }
    unsigned int  GetPDataOff() const { return pdataOff; }

    void          AddNode(XMLTreeNode *node, addmode mode);
    XMLTreeNode  *AddNode(addmode mode);

    void          SetNext(XMLTreeNode *nxt);
    void          SetChild(XMLTreeNode *cld);
    void          SetParent(XMLTreeNode *prnt);

    void          SetAttribute(char *name, char *value);
    void          SetType(char *typ);
    void          SetData(char *dat, unsigned int datSize);
    void          AppendData(char *dat, unsigned int datSize);
    void          SetPDataOff(int pdo);

    void          SetMatchingMode(matchmode mode);

    void          DeleteAttribute(char *name);
    void          DeleteAttributes();
    void          DeleteChildren();
};

class XMLTreeParser:public XML_Parser
{
  protected:
    XMLTreeNode *root, *current;

    virtual void StartElementHandler(const XML_Char *name, const XML_Char **atts);
    virtual void EndElementHandler(const XML_Char *name);
    virtual void CharacterDataHandler(const XML_Char *s, int len);

  public:
    XMLTreeParser(const XML_Char *encoding);
    virtual ~XMLTreeParser();

    XMLTreeNode *RootNode() { return root; };
};

#endif // __XMLTREE_H
