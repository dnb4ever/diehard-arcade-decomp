/* 0x2217d22  bucket-list remove: unlink obj from its spatial-grid bucket list
   (head at *obj->f40, next pointer at node->f44), then clear obj->f0.
   NONMATCHING: SHC r32 numbers the scratch registers differently from the 1996
   build (identical control flow otherwise). */
typedef int s32;
typedef struct Obj {
    s32 f0;  s32 f4;  s32 *f8; s32 fC;
    s32 f10; s32 f14; s32 f18; s32 f1C;
    s32 f20; s32 f24; s32 f28; s32 f2C;
    s32 f30; s32 f34; s32 f38; s32 f3C;
    s32 f40; s32 f44;
} Obj;

void f_217d22(Obj *e) {
    Obj **pp = (Obj **)e->f40;
    Obj *head = *pp;
    if (head != 0) {
        if (head == e) {
            *pp = (Obj *)e->f44;
        } else {
            while ((Obj *)(*pp)->f44 != 0) {
                if ((Obj *)(*pp)->f44 == e) {
                    (*pp)->f44 = e->f44;
                    break;
                }
                pp = (Obj **)&(*pp)->f44;
            }
        }
    }
    e->f0 = 0;
}
