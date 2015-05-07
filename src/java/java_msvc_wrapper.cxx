/*
 * java_msvc_wrapper.c
 *
 * Java interface for OPAL
 *
 * This primarily exists for the MSVC environment as when you go "Rebuild All"
 * it will delete all output products, including the java_swig_wrapper.cxx
 * from the custom build stage that runs swig.exe. All is fine if you have a
 * swig.exe, but if you don't it then it can't be regenerated. We would like
 * people without swig.exe to still be able to build the system, so we
 * insulate the .cxx file, which is provided in SVN and tarballs, from being
 * deleted by copying to a .inc file, and including that here.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <opal_config.h>

#if OPAL_JAVA
#include "java_swig_wrapper.inc"
#endif

