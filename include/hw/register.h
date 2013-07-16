/*
 * register.h
 *
 *  Created on: Jul 3, 2013
 *      Author: andre
 */

#ifndef _REGISTER_H
#define _REGISTER_H

#include "hw/qdev.h"

#define TYPE_REGISTER_FILE "register-file-device"

typedef struct RegisterFile RegisterFile;

#define REGISTER_FILE_CLASS(klass) \
        OBJECT_CLASS_CHECK(RegisterFileClass, klass, TYPE_REGISTER_FILE)
#define REGISTER_FILE(obj) \
        OBJECT_CHECK(RegisterFile, (obj), TYPE_REGISTER_FILE)

#endif
