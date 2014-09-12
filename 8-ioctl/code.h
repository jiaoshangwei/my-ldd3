

#ifndef __CODE_H__

#define __CODE_H__

#define JTYPE 88

#define J_IO            _IO(JTYPE,0)
#define J_WRITE_VALUE   _IOW(JTYPE,1,ulong)
#define J_WRITE_POINTER _IOW(JTYPE,2, ulong *)
#define J_READ_VALUE    _IOR(JTYPE,3,ulong)
#define J_READ_POINTER  _IOR(JTYPE,4,ulong *)


#endif
