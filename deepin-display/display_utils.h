#ifndef DISPLAYUTILS_H
#define DISPLAYUTILS_H

#define ADD_PROPERTY(ptype, pname) \
Q_PROPERTY(ptype pname READ pname WRITE set##pname) \
ptype m_##pname; \
virtual ptype pname() = 0; \
virtual void set##pname(ptype) = 0;


#endif