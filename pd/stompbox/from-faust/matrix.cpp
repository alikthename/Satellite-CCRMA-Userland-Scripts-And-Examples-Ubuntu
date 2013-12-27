//-----------------------------------------------------
// name: "matrix"
// version: "1.0"
// author: "Grame"
// license: "BSD"
// copyright: "(c)GRAME 2006"
//
// Code generated with Faust 0.9.58 (http://faust.grame.fr)
//-----------------------------------------------------
/* link with  */
/************************************************************************
 ************************************************************************
    FAUST Architecture File
	Copyright (C) 2006-2011 Albert Graef <Dr.Graef@t-online.de>
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
	published by the Free Software Foundation; either version 2.1 of the 
	License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
 	License along with the GNU C Library; if not, write to the Free
  	Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  	02111-1307 USA. 
 ************************************************************************
 ************************************************************************/

/* Pd architecture file, written by Albert Graef <Dr.Graef@t-online.de>.
   This was derived from minimal.cpp included in the Faust distribution.
   Please note that this is to be compiled as a shared library, which is
   then loaded dynamically by Pd as an external. */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>

using namespace std;

// On Intel set FZ (Flush to Zero) and DAZ (Denormals Are Zero)
// flags to avoid costly denormals
#ifdef __SSE__
    #include <xmmintrin.h>
    #ifdef __SSE2__
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8040)
    #else
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8000)
    #endif
#else
  #warning *** puredata.cpp: NO SSE FLAG (denormals may slow things down) ***
  #define AVOIDDENORMALS
#endif

struct Meta 
{
    void declare (const char* key, const char* value) {}
};


// abs is now predefined
//template<typename T> T abs (T a)			{ return (a<T(0)) ? -a : a; }


inline int		lsr (int x, int n)			{ return int(((unsigned int)x) >> n); }

/******************************************************************************
*******************************************************************************

							       VECTOR INTRINSICS

*******************************************************************************
*******************************************************************************/

//inline void *aligned_calloc(size_t nmemb, size_t size) { return (void*)((unsigned)(calloc((nmemb*size)+15,sizeof(char)))+15 & 0xfffffff0); }
//inline void *aligned_calloc(size_t nmemb, size_t size) { return (void*)((size_t)(calloc((nmemb*size)+15,sizeof(char)))+15 & ~15); }


/******************************************************************************
*******************************************************************************

			ABSTRACT USER INTERFACE

*******************************************************************************
*******************************************************************************/

class UI
{
  bool	fStopped;
public:
		
  UI() : fStopped(false) {}
  virtual ~UI() {}
	
  virtual void addButton(const char* label, float* zone) = 0;
  virtual void addCheckButton(const char* label, float* zone) = 0;
  virtual void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step) = 0;
  virtual void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step) = 0;
  virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step) = 0;

  virtual void addHorizontalBargraph(const char* label, float* zone, float min, float max) = 0;
  virtual void addVerticalBargraph(const char* label, float* zone, float min, float max) = 0;
	
  virtual void openTabBox(const char* label) = 0;
  virtual void openHorizontalBox(const char* label) = 0;
  virtual void openVerticalBox(const char* label) = 0;
  virtual void closeBox() = 0;
	
  virtual void run() = 0;
	
  void stop()	{ fStopped = true; }
  bool stopped() 	{ return fStopped; }

  virtual void declare(float* zone, const char* key, const char* value) {}
};

/***************************************************************************
   Pd UI interface
 ***************************************************************************/

enum ui_elem_type_t {
  UI_BUTTON, UI_CHECK_BUTTON,
  UI_V_SLIDER, UI_H_SLIDER, UI_NUM_ENTRY,
  UI_V_BARGRAPH, UI_H_BARGRAPH,
  UI_END_GROUP, UI_V_GROUP, UI_H_GROUP, UI_T_GROUP
};

struct ui_elem_t {
  ui_elem_type_t type;
  char *label;
  float *zone;
  float init, min, max, step;
};

class PdUI : public UI
{
public:
  int nelems;
  ui_elem_t *elems;
		
  PdUI();
  PdUI(const char *s);
  virtual ~PdUI();

protected:
  string path;
  void add_elem(ui_elem_type_t type, const char *label = NULL);
  void add_elem(ui_elem_type_t type, const char *label, float *zone);
  void add_elem(ui_elem_type_t type, const char *label, float *zone,
		float init, float min, float max, float step);
  void add_elem(ui_elem_type_t type, const char *label, float *zone,
		float min, float max);

public:
  virtual void addButton(const char* label, float* zone);
  virtual void addCheckButton(const char* label, float* zone);
  virtual void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step);
  virtual void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step);
  virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step);

  virtual void addHorizontalBargraph(const char* label, float* zone, float min, float max);
  virtual void addVerticalBargraph(const char* label, float* zone, float min, float max);
  
  virtual void openTabBox(const char* label);
  virtual void openHorizontalBox(const char* label);
  virtual void openVerticalBox(const char* label);
  virtual void closeBox();
	
  virtual void run();
};

static string mangle(const char *s)
{
  const char *s0 = s;
  string t = "";
  if (!s) return t;
  while (*s)
    if (isalnum(*s))
      t += *(s++);
    else {
      const char *s1 = s;
      while (*s && !isalnum(*s)) ++s;
      if (s1 != s0 && *s) t += "-";
    }
  return t;
}

static string normpath(string path)
{
  path = string("/")+path;
  int pos = path.find("//");
  while (pos >= 0) {
    path.erase(pos, 1);
    pos = path.find("//");
  }
  return path;
}

static string pathcat(string path, string label)
{
  if (path.empty())
    return normpath(label);
  else if (label.empty())
    return normpath(path);
  else
    return normpath(path+"/"+label);
}

PdUI::PdUI()
{
  nelems = 0;
  elems = NULL;
  path = "";
}

PdUI::PdUI(const char *s)
{
  nelems = 0;
  elems = NULL;
  path = s?s:"";
}

PdUI::~PdUI()
{
  if (elems) {
    for (int i = 0; i < nelems; i++)
      if (elems[i].label)
	free(elems[i].label);
    free(elems);
  }
}

inline void PdUI::add_elem(ui_elem_type_t type, const char *label)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  string s = pathcat(path, mangle(label));
  elems[nelems].type = type;
  elems[nelems].label = strdup(s.c_str());
  elems[nelems].zone = NULL;
  elems[nelems].init = 0.0;
  elems[nelems].min = 0.0;
  elems[nelems].max = 0.0;
  elems[nelems].step = 0.0;
  nelems++;
}

inline void PdUI::add_elem(ui_elem_type_t type, const char *label, float *zone)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  string s = pathcat(path, mangle(label));
  elems[nelems].type = type;
  elems[nelems].label = strdup(s.c_str());
  elems[nelems].zone = zone;
  elems[nelems].init = 0.0;
  elems[nelems].min = 0.0;
  elems[nelems].max = 1.0;
  elems[nelems].step = 1.0;
  nelems++;
}

inline void PdUI::add_elem(ui_elem_type_t type, const char *label, float *zone,
			  float init, float min, float max, float step)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  string s = pathcat(path, mangle(label));
  elems[nelems].type = type;
  elems[nelems].label = strdup(s.c_str());
  elems[nelems].zone = zone;
  elems[nelems].init = init;
  elems[nelems].min = min;
  elems[nelems].max = max;
  elems[nelems].step = step;
  nelems++;
}

inline void PdUI::add_elem(ui_elem_type_t type, const char *label, float *zone,
			  float min, float max)
{
  ui_elem_t *elems1 = (ui_elem_t*)realloc(elems, (nelems+1)*sizeof(ui_elem_t));
  if (elems1)
    elems = elems1;
  else
    return;
  string s = pathcat(path, mangle(label));
  elems[nelems].type = type;
  elems[nelems].label = strdup(s.c_str());
  elems[nelems].zone = zone;
  elems[nelems].init = 0.0;
  elems[nelems].min = min;
  elems[nelems].max = max;
  elems[nelems].step = 0.0;
  nelems++;
}

void PdUI::addButton(const char* label, float* zone)
{ add_elem(UI_BUTTON, label, zone); }
void PdUI::addCheckButton(const char* label, float* zone)
{ add_elem(UI_CHECK_BUTTON, label, zone); }
void PdUI::addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step)
{ add_elem(UI_V_SLIDER, label, zone, init, min, max, step); }
void PdUI::addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step)
{ add_elem(UI_H_SLIDER, label, zone, init, min, max, step); }
void PdUI::addNumEntry(const char* label, float* zone, float init, float min, float max, float step)
{ add_elem(UI_NUM_ENTRY, label, zone, init, min, max, step); }

void PdUI::addHorizontalBargraph(const char* label, float* zone, float min, float max)
{ add_elem(UI_H_BARGRAPH, label, zone, min, max); }
void PdUI::addVerticalBargraph(const char* label, float* zone, float min, float max)
{ add_elem(UI_V_BARGRAPH, label, zone, min, max); }

void PdUI::openTabBox(const char* label)
{
  if (!path.empty()) path += "/";
  path += mangle(label);
}
void PdUI::openHorizontalBox(const char* label)
{
  if (!path.empty()) path += "/";
  path += mangle(label);
}
void PdUI::openVerticalBox(const char* label)
{
  if (!path.empty()) path += "/";
  path += mangle(label);
}
void PdUI::closeBox()
{
  int pos = path.rfind("/");
  if (pos < 0) pos = 0;
  path.erase(pos);
}

void PdUI::run() {}

/******************************************************************************
*******************************************************************************

			    FAUST DSP

*******************************************************************************
*******************************************************************************/



//----------------------------------------------------------------
//  abstract definition of a signal processor
//----------------------------------------------------------------
			
class dsp {
 protected:
	int fSamplingFreq;
 public:
	dsp() {}
	virtual ~dsp() {}
	virtual int getNumInputs() = 0;
	virtual int getNumOutputs() = 0;
	virtual void buildUserInterface(UI* interface) = 0;
	virtual void init(int samplingRate) = 0;
 	virtual void compute(int len, float** inputs, float** outputs) = 0;
};

//----------------------------------------------------------------------------
//  FAUST generated signal processor
//----------------------------------------------------------------------------
		

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif  

typedef long double quad;

#ifndef FAUSTCLASS 
#define FAUSTCLASS mydsp
#endif

class mydsp : public dsp {
  private:
	FAUSTFLOAT 	fslider0;
	FAUSTFLOAT 	fslider1;
	FAUSTFLOAT 	fslider2;
	FAUSTFLOAT 	fslider3;
	FAUSTFLOAT 	fslider4;
	FAUSTFLOAT 	fslider5;
	FAUSTFLOAT 	fslider6;
	FAUSTFLOAT 	fslider7;
	FAUSTFLOAT 	fslider8;
	FAUSTFLOAT 	fslider9;
	FAUSTFLOAT 	fslider10;
	FAUSTFLOAT 	fslider11;
	FAUSTFLOAT 	fslider12;
	FAUSTFLOAT 	fslider13;
	FAUSTFLOAT 	fslider14;
	FAUSTFLOAT 	fslider15;
	FAUSTFLOAT 	fslider16;
	FAUSTFLOAT 	fslider17;
	FAUSTFLOAT 	fslider18;
	FAUSTFLOAT 	fslider19;
	FAUSTFLOAT 	fslider20;
	FAUSTFLOAT 	fslider21;
	FAUSTFLOAT 	fslider22;
	FAUSTFLOAT 	fslider23;
	FAUSTFLOAT 	fslider24;
	FAUSTFLOAT 	fslider25;
	FAUSTFLOAT 	fslider26;
	FAUSTFLOAT 	fslider27;
	FAUSTFLOAT 	fslider28;
	FAUSTFLOAT 	fslider29;
	FAUSTFLOAT 	fslider30;
	FAUSTFLOAT 	fslider31;
	FAUSTFLOAT 	fslider32;
	FAUSTFLOAT 	fslider33;
	FAUSTFLOAT 	fslider34;
	FAUSTFLOAT 	fslider35;
	FAUSTFLOAT 	fslider36;
	FAUSTFLOAT 	fslider37;
	FAUSTFLOAT 	fslider38;
	FAUSTFLOAT 	fslider39;
	FAUSTFLOAT 	fslider40;
	FAUSTFLOAT 	fslider41;
	FAUSTFLOAT 	fslider42;
	FAUSTFLOAT 	fslider43;
	FAUSTFLOAT 	fslider44;
	FAUSTFLOAT 	fslider45;
	FAUSTFLOAT 	fslider46;
	FAUSTFLOAT 	fslider47;
	FAUSTFLOAT 	fslider48;
	FAUSTFLOAT 	fslider49;
	FAUSTFLOAT 	fslider50;
	FAUSTFLOAT 	fslider51;
	FAUSTFLOAT 	fslider52;
	FAUSTFLOAT 	fslider53;
	FAUSTFLOAT 	fslider54;
	FAUSTFLOAT 	fslider55;
	FAUSTFLOAT 	fslider56;
	FAUSTFLOAT 	fslider57;
	FAUSTFLOAT 	fslider58;
	FAUSTFLOAT 	fslider59;
	FAUSTFLOAT 	fslider60;
	FAUSTFLOAT 	fslider61;
	FAUSTFLOAT 	fslider62;
	FAUSTFLOAT 	fslider63;
  public:
	static void metadata(Meta* m) 	{ 
		m->declare("name", "matrix");
		m->declare("version", "1.0");
		m->declare("author", "Grame");
		m->declare("license", "BSD");
		m->declare("copyright", "(c)GRAME 2006");
		m->declare("music.lib/name", "Music Library");
		m->declare("music.lib/author", "GRAME");
		m->declare("music.lib/copyright", "GRAME");
		m->declare("music.lib/version", "1.0");
		m->declare("music.lib/license", "LGPL with exception");
		m->declare("math.lib/name", "Math Library");
		m->declare("math.lib/author", "GRAME");
		m->declare("math.lib/copyright", "GRAME");
		m->declare("math.lib/version", "1.0");
		m->declare("math.lib/license", "LGPL with exception");
	}

	virtual int getNumInputs() 	{ return 8; }
	virtual int getNumOutputs() 	{ return 8; }
	static void classInit(int samplingFreq) {
	}
	virtual void instanceInit(int samplingFreq) {
		fSamplingFreq = samplingFreq;
		fslider0 = -1e+01f;
		fslider1 = -1e+01f;
		fslider2 = -1e+01f;
		fslider3 = -1e+01f;
		fslider4 = -1e+01f;
		fslider5 = -1e+01f;
		fslider6 = -1e+01f;
		fslider7 = -1e+01f;
		fslider8 = -1e+01f;
		fslider9 = -1e+01f;
		fslider10 = -1e+01f;
		fslider11 = -1e+01f;
		fslider12 = -1e+01f;
		fslider13 = -1e+01f;
		fslider14 = -1e+01f;
		fslider15 = -1e+01f;
		fslider16 = -1e+01f;
		fslider17 = -1e+01f;
		fslider18 = -1e+01f;
		fslider19 = -1e+01f;
		fslider20 = -1e+01f;
		fslider21 = -1e+01f;
		fslider22 = -1e+01f;
		fslider23 = -1e+01f;
		fslider24 = -1e+01f;
		fslider25 = -1e+01f;
		fslider26 = -1e+01f;
		fslider27 = -1e+01f;
		fslider28 = -1e+01f;
		fslider29 = -1e+01f;
		fslider30 = -1e+01f;
		fslider31 = -1e+01f;
		fslider32 = -1e+01f;
		fslider33 = -1e+01f;
		fslider34 = -1e+01f;
		fslider35 = -1e+01f;
		fslider36 = -1e+01f;
		fslider37 = -1e+01f;
		fslider38 = -1e+01f;
		fslider39 = -1e+01f;
		fslider40 = -1e+01f;
		fslider41 = -1e+01f;
		fslider42 = -1e+01f;
		fslider43 = -1e+01f;
		fslider44 = -1e+01f;
		fslider45 = -1e+01f;
		fslider46 = -1e+01f;
		fslider47 = -1e+01f;
		fslider48 = -1e+01f;
		fslider49 = -1e+01f;
		fslider50 = -1e+01f;
		fslider51 = -1e+01f;
		fslider52 = -1e+01f;
		fslider53 = -1e+01f;
		fslider54 = -1e+01f;
		fslider55 = -1e+01f;
		fslider56 = -1e+01f;
		fslider57 = -1e+01f;
		fslider58 = -1e+01f;
		fslider59 = -1e+01f;
		fslider60 = -1e+01f;
		fslider61 = -1e+01f;
		fslider62 = -1e+01f;
		fslider63 = -1e+01f;
	}
	virtual void init(int samplingFreq) {
		classInit(samplingFreq);
		instanceInit(samplingFreq);
	}
	virtual void buildUserInterface(UI* interface) {
		interface->openTabBox("Matrix 8 x 8");
		interface->openHorizontalBox("Output 0");
		interface->addVerticalSlider("Input 0", &fslider0, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider1, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider2, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider3, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider4, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider5, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider6, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider7, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->openHorizontalBox("Output 1");
		interface->addVerticalSlider("Input 0", &fslider8, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider9, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider10, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider11, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider12, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider13, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider14, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider15, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->openHorizontalBox("Output 2");
		interface->addVerticalSlider("Input 0", &fslider16, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider17, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider18, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider19, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider20, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider21, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider22, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider23, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->openHorizontalBox("Output 3");
		interface->addVerticalSlider("Input 0", &fslider24, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider25, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider26, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider27, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider28, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider29, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider30, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider31, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->openHorizontalBox("Output 4");
		interface->addVerticalSlider("Input 0", &fslider32, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider33, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider34, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider35, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider36, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider37, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider38, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider39, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->openHorizontalBox("Output 5");
		interface->addVerticalSlider("Input 0", &fslider40, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider41, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider42, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider43, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider44, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider45, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider46, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider47, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->openHorizontalBox("Output 6");
		interface->addVerticalSlider("Input 0", &fslider48, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider49, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider50, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider51, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider52, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider53, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider54, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider55, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->openHorizontalBox("Output 7");
		interface->addVerticalSlider("Input 0", &fslider56, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 1", &fslider57, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 2", &fslider58, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 3", &fslider59, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 4", &fslider60, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 5", &fslider61, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 6", &fslider62, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->addVerticalSlider("Input 7", &fslider63, -1e+01f, -96.0f, 4.0f, 0.1f);
		interface->closeBox();
		interface->closeBox();
	}
	virtual void compute (int count, FAUSTFLOAT** input, FAUSTFLOAT** output) {
		float 	fSlow0 = powf(10,(0.05f * fslider0));
		float 	fSlow1 = powf(10,(0.05f * fslider1));
		float 	fSlow2 = powf(10,(0.05f * fslider2));
		float 	fSlow3 = powf(10,(0.05f * fslider3));
		float 	fSlow4 = powf(10,(0.05f * fslider4));
		float 	fSlow5 = powf(10,(0.05f * fslider5));
		float 	fSlow6 = powf(10,(0.05f * fslider6));
		float 	fSlow7 = powf(10,(0.05f * fslider7));
		float 	fSlow8 = powf(10,(0.05f * fslider8));
		float 	fSlow9 = powf(10,(0.05f * fslider9));
		float 	fSlow10 = powf(10,(0.05f * fslider10));
		float 	fSlow11 = powf(10,(0.05f * fslider11));
		float 	fSlow12 = powf(10,(0.05f * fslider12));
		float 	fSlow13 = powf(10,(0.05f * fslider13));
		float 	fSlow14 = powf(10,(0.05f * fslider14));
		float 	fSlow15 = powf(10,(0.05f * fslider15));
		float 	fSlow16 = powf(10,(0.05f * fslider16));
		float 	fSlow17 = powf(10,(0.05f * fslider17));
		float 	fSlow18 = powf(10,(0.05f * fslider18));
		float 	fSlow19 = powf(10,(0.05f * fslider19));
		float 	fSlow20 = powf(10,(0.05f * fslider20));
		float 	fSlow21 = powf(10,(0.05f * fslider21));
		float 	fSlow22 = powf(10,(0.05f * fslider22));
		float 	fSlow23 = powf(10,(0.05f * fslider23));
		float 	fSlow24 = powf(10,(0.05f * fslider24));
		float 	fSlow25 = powf(10,(0.05f * fslider25));
		float 	fSlow26 = powf(10,(0.05f * fslider26));
		float 	fSlow27 = powf(10,(0.05f * fslider27));
		float 	fSlow28 = powf(10,(0.05f * fslider28));
		float 	fSlow29 = powf(10,(0.05f * fslider29));
		float 	fSlow30 = powf(10,(0.05f * fslider30));
		float 	fSlow31 = powf(10,(0.05f * fslider31));
		float 	fSlow32 = powf(10,(0.05f * fslider32));
		float 	fSlow33 = powf(10,(0.05f * fslider33));
		float 	fSlow34 = powf(10,(0.05f * fslider34));
		float 	fSlow35 = powf(10,(0.05f * fslider35));
		float 	fSlow36 = powf(10,(0.05f * fslider36));
		float 	fSlow37 = powf(10,(0.05f * fslider37));
		float 	fSlow38 = powf(10,(0.05f * fslider38));
		float 	fSlow39 = powf(10,(0.05f * fslider39));
		float 	fSlow40 = powf(10,(0.05f * fslider40));
		float 	fSlow41 = powf(10,(0.05f * fslider41));
		float 	fSlow42 = powf(10,(0.05f * fslider42));
		float 	fSlow43 = powf(10,(0.05f * fslider43));
		float 	fSlow44 = powf(10,(0.05f * fslider44));
		float 	fSlow45 = powf(10,(0.05f * fslider45));
		float 	fSlow46 = powf(10,(0.05f * fslider46));
		float 	fSlow47 = powf(10,(0.05f * fslider47));
		float 	fSlow48 = powf(10,(0.05f * fslider48));
		float 	fSlow49 = powf(10,(0.05f * fslider49));
		float 	fSlow50 = powf(10,(0.05f * fslider50));
		float 	fSlow51 = powf(10,(0.05f * fslider51));
		float 	fSlow52 = powf(10,(0.05f * fslider52));
		float 	fSlow53 = powf(10,(0.05f * fslider53));
		float 	fSlow54 = powf(10,(0.05f * fslider54));
		float 	fSlow55 = powf(10,(0.05f * fslider55));
		float 	fSlow56 = powf(10,(0.05f * fslider56));
		float 	fSlow57 = powf(10,(0.05f * fslider57));
		float 	fSlow58 = powf(10,(0.05f * fslider58));
		float 	fSlow59 = powf(10,(0.05f * fslider59));
		float 	fSlow60 = powf(10,(0.05f * fslider60));
		float 	fSlow61 = powf(10,(0.05f * fslider61));
		float 	fSlow62 = powf(10,(0.05f * fslider62));
		float 	fSlow63 = powf(10,(0.05f * fslider63));
		FAUSTFLOAT* input0 = input[0];
		FAUSTFLOAT* input1 = input[1];
		FAUSTFLOAT* input2 = input[2];
		FAUSTFLOAT* input3 = input[3];
		FAUSTFLOAT* input4 = input[4];
		FAUSTFLOAT* input5 = input[5];
		FAUSTFLOAT* input6 = input[6];
		FAUSTFLOAT* input7 = input[7];
		FAUSTFLOAT* output0 = output[0];
		FAUSTFLOAT* output1 = output[1];
		FAUSTFLOAT* output2 = output[2];
		FAUSTFLOAT* output3 = output[3];
		FAUSTFLOAT* output4 = output[4];
		FAUSTFLOAT* output5 = output[5];
		FAUSTFLOAT* output6 = output[6];
		FAUSTFLOAT* output7 = output[7];
		for (int i=0; i<count; i++) {
			float fTemp0 = (float)input0[i];
			float fTemp1 = (float)input1[i];
			float fTemp2 = (float)input2[i];
			float fTemp3 = (float)input3[i];
			float fTemp4 = (float)input4[i];
			float fTemp5 = (float)input5[i];
			float fTemp6 = (float)input6[i];
			float fTemp7 = (float)input7[i];
			output0[i] = (FAUSTFLOAT)((((((((fSlow0 * fTemp0) + (fSlow1 * fTemp1)) + (fSlow2 * fTemp2)) + (fSlow3 * fTemp3)) + (fSlow4 * fTemp4)) + (fSlow5 * fTemp5)) + (fSlow6 * fTemp6)) + (fSlow7 * fTemp7));
			output1[i] = (FAUSTFLOAT)((((((((fSlow8 * fTemp0) + (fSlow9 * fTemp1)) + (fSlow10 * fTemp2)) + (fSlow11 * fTemp3)) + (fSlow12 * fTemp4)) + (fSlow13 * fTemp5)) + (fSlow14 * fTemp6)) + (fSlow15 * fTemp7));
			output2[i] = (FAUSTFLOAT)((((((((fSlow16 * fTemp0) + (fSlow17 * fTemp1)) + (fSlow18 * fTemp2)) + (fSlow19 * fTemp3)) + (fSlow20 * fTemp4)) + (fSlow21 * fTemp5)) + (fSlow22 * fTemp6)) + (fSlow23 * fTemp7));
			output3[i] = (FAUSTFLOAT)((((((((fSlow24 * fTemp0) + (fSlow25 * fTemp1)) + (fSlow26 * fTemp2)) + (fSlow27 * fTemp3)) + (fSlow28 * fTemp4)) + (fSlow29 * fTemp5)) + (fSlow30 * fTemp6)) + (fSlow31 * fTemp7));
			output4[i] = (FAUSTFLOAT)((((((((fSlow32 * fTemp0) + (fSlow33 * fTemp1)) + (fSlow34 * fTemp2)) + (fSlow35 * fTemp3)) + (fSlow36 * fTemp4)) + (fSlow37 * fTemp5)) + (fSlow38 * fTemp6)) + (fSlow39 * fTemp7));
			output5[i] = (FAUSTFLOAT)((((((((fSlow40 * fTemp0) + (fSlow41 * fTemp1)) + (fSlow42 * fTemp2)) + (fSlow43 * fTemp3)) + (fSlow44 * fTemp4)) + (fSlow45 * fTemp5)) + (fSlow46 * fTemp6)) + (fSlow47 * fTemp7));
			output6[i] = (FAUSTFLOAT)((((((((fSlow48 * fTemp0) + (fSlow49 * fTemp1)) + (fSlow50 * fTemp2)) + (fSlow51 * fTemp3)) + (fSlow52 * fTemp4)) + (fSlow53 * fTemp5)) + (fSlow54 * fTemp6)) + (fSlow55 * fTemp7));
			output7[i] = (FAUSTFLOAT)((((((((fSlow56 * fTemp0) + (fSlow57 * fTemp1)) + (fSlow58 * fTemp2)) + (fSlow59 * fTemp3)) + (fSlow60 * fTemp4)) + (fSlow61 * fTemp5)) + (fSlow62 * fTemp6)) + (fSlow63 * fTemp7));
		}
	}
};



#include <stdio.h>
#include <string.h>
#include "m_pd.h"

#define faust_setup(name) xfaust_setup(name)
#define xfaust_setup(name) name ## _tilde_setup(void)
#define sym(name) xsym(name)
#define xsym(name) #name

// time for "active" toggle xfades in secs
#define XFADE_TIME 0.1f

static t_class *faust_class;

struct t_faust {
  t_object x_obj;
#ifdef __MINGW32__
  /* This seems to be necessary as some as yet undetermined Pd routine seems
     to write past the end of x_obj on Windows. */
  int fence; /* dummy field (not used) */
#endif
  mydsp *dsp;
  PdUI *ui;
  string *label;
  int active, xfade, n_xfade, rate, n_in, n_out;
  t_sample **inputs, **outputs, **buf;
  t_outlet *out;
  t_sample f;
};

static t_symbol *s_button, *s_checkbox, *s_vslider, *s_hslider, *s_nentry,
  *s_vbargraph, *s_hbargraph;

static inline void zero_samples(int k, int n, t_sample **out)
{
  for (int i = 0; i < k; i++)
#ifdef __STDC_IEC_559__
    /* IEC 559 a.k.a. IEEE 754 floats can be initialized faster like this */
    memset(out[i], 0, n*sizeof(t_sample));
#else
    for (int j = 0; j < n; j++)
      out[i][j] = 0.0f;
#endif
}

static inline void copy_samples(int k, int n, t_sample **out, t_sample **in)
{
  for (int i = 0; i < k; i++)
    memcpy(out[i], in[i], n*sizeof(t_sample));
}

static t_int *faust_perform(t_int *w)
{
  t_faust *x = (t_faust *)(w[1]);
  int n = (int)(w[2]);
  if (!x->dsp || !x->buf) return (w+3);
  AVOIDDENORMALS;
  if (x->xfade > 0) {
    float d = 1.0f/x->n_xfade, f = (x->xfade--)*d;
    d = d/n;
    x->dsp->compute(n, x->inputs, x->buf);
    if (x->active)
      if (x->n_in == x->n_out)
	/* xfade inputs -> buf */
	for (int j = 0; j < n; j++, f -= d)
	  for (int i = 0; i < x->n_out; i++)
	    x->outputs[i][j] = f*x->inputs[i][j]+(1.0f-f)*x->buf[i][j];
      else
	/* xfade 0 -> buf */
	for (int j = 0; j < n; j++, f -= d)
	  for (int i = 0; i < x->n_out; i++)
	    x->outputs[i][j] = (1.0f-f)*x->buf[i][j];
    else
      if (x->n_in == x->n_out)
	/* xfade buf -> inputs */
	for (int j = 0; j < n; j++, f -= d)
	  for (int i = 0; i < x->n_out; i++)
	    x->outputs[i][j] = f*x->buf[i][j]+(1.0f-f)*x->inputs[i][j];
      else
	/* xfade buf -> 0 */
	for (int j = 0; j < n; j++, f -= d)
	  for (int i = 0; i < x->n_out; i++)
	    x->outputs[i][j] = f*x->buf[i][j];
  } else if (x->active) {
    x->dsp->compute(n, x->inputs, x->buf);
    copy_samples(x->n_out, n, x->outputs, x->buf);
  } else if (x->n_in == x->n_out) {
    copy_samples(x->n_out, n, x->buf, x->inputs);
    copy_samples(x->n_out, n, x->outputs, x->buf);
  } else
    zero_samples(x->n_out, n, x->outputs);
  return (w+3);
}

static void faust_dsp(t_faust *x, t_signal **sp)
{
  int n = sp[0]->s_n, sr = (int)sp[0]->s_sr;
  if (x->rate <= 0) {
    /* default sample rate is whatever Pd tells us */
    PdUI *ui = x->ui;
    float *z = NULL;
    if (ui->nelems > 0 &&
	(z = (float*)malloc(ui->nelems*sizeof(float)))) {
      /* save the current control values */
      for (int i = 0; i < ui->nelems; i++)
	if (ui->elems[i].zone)
	  z[i] = *ui->elems[i].zone;
    }
    /* set the proper sample rate; this requires reinitializing the dsp */
    x->rate = sr;
    x->dsp->init(sr);
    if (z) {
      /* restore previous control values */
      for (int i = 0; i < ui->nelems; i++)
	if (ui->elems[i].zone)
	  *ui->elems[i].zone = z[i];
      free(z);
    }
  }
  if (n > 0)
    x->n_xfade = (int)(x->rate*XFADE_TIME/n);
  dsp_add(faust_perform, 2, x, n);
  for (int i = 0; i < x->n_in; i++)
    x->inputs[i] = sp[i+1]->s_vec;
  for (int i = 0; i < x->n_out; i++)
    x->outputs[i] = sp[x->n_in+i+1]->s_vec;
  if (x->buf != NULL)
    for (int i = 0; i < x->n_out; i++) {
      x->buf[i] = (t_sample*)malloc(n*sizeof(t_sample));
      if (x->buf[i] == NULL) {
	for (int j = 0; j < i; j++)
	  free(x->buf[j]);
	free(x->buf);
	x->buf = NULL;
	break;
      }
    }
}

static int pathcmp(const char *s, const char *t)
{
  int n = strlen(s), m = strlen(t);
  if (n == 0 || m == 0)
    return 0;
  else if (t[0] == '/')
    return strcmp(s, t);
  else if (n <= m || s[n-m-1] != '/')
    return strcmp(s+1, t);
  else
    return strcmp(s+n-m, t);
}

static void faust_any(t_faust *x, t_symbol *s, int argc, t_atom *argv)
{
  if (!x->dsp) return;
  PdUI *ui = x->ui;
  if (s == &s_bang) {
    for (int i = 0; i < ui->nelems; i++)
      if (ui->elems[i].label && ui->elems[i].zone) {
	t_atom args[6];
	t_symbol *_s;
	switch (ui->elems[i].type) {
	case UI_BUTTON:
	  _s = s_button;
	  break;
	case UI_CHECK_BUTTON:
	  _s = s_checkbox;
	  break;
	case UI_V_SLIDER:
	  _s = s_vslider;
	  break;
	case UI_H_SLIDER:
	  _s = s_hslider;
	  break;
	case UI_NUM_ENTRY:
	  _s = s_nentry;
	  break;
	case UI_V_BARGRAPH:
	  _s = s_vbargraph;
	  break;
	case UI_H_BARGRAPH:
	  _s = s_hbargraph;
	  break;
	default:
	  continue;
	}
	SETSYMBOL(&args[0], gensym(ui->elems[i].label));
	SETFLOAT(&args[1], *ui->elems[i].zone);
	SETFLOAT(&args[2], ui->elems[i].init);
	SETFLOAT(&args[3], ui->elems[i].min);
	SETFLOAT(&args[4], ui->elems[i].max);
	SETFLOAT(&args[5], ui->elems[i].step);
	outlet_anything(x->out, _s, 6, args);
      }
  } else {
    const char *label = s->s_name;
    int count = 0;
    for (int i = 0; i < ui->nelems; i++)
      if (ui->elems[i].label &&
	  pathcmp(ui->elems[i].label, label) == 0) {
	if (argc == 0) {
	  if (ui->elems[i].zone) {
	    t_atom arg;
	    SETFLOAT(&arg, *ui->elems[i].zone);
	    outlet_anything(x->out, gensym(ui->elems[i].label), 1, &arg);
	  }
	  ++count;
	} else if (argc == 1 &&
		   (argv[0].a_type == A_FLOAT ||
		    argv[0].a_type == A_DEFFLOAT) &&
		   ui->elems[i].zone) {
	  float f = atom_getfloat(argv);
	  *ui->elems[i].zone = f;
	  ++count;
	} else
	  pd_error(x, "[faust] %s: bad control argument: %s",
		   x->label->c_str(), label);
      }
    if (count == 0 && strcmp(label, "active") == 0) {
      if (argc == 0) {
	t_atom arg;
	SETFLOAT(&arg, (float)x->active);
	outlet_anything(x->out, gensym((char*)"active"), 1, &arg);
      } else if (argc == 1 &&
		 (argv[0].a_type == A_FLOAT ||
		  argv[0].a_type == A_DEFFLOAT)) {
	float f = atom_getfloat(argv);
	x->active = (int)f;
	x->xfade = x->n_xfade;
      }
    }
  }
}

static void faust_free(t_faust *x)
{
  if (x->label) delete x->label;
  if (x->dsp) delete x->dsp;
  if (x->ui) delete x->ui;
  if (x->inputs) free(x->inputs);
  if (x->outputs) free(x->outputs);
  if (x->buf) {
    for (int i = 0; i < x->n_out; i++)
      if (x->buf[i]) free(x->buf[i]);
    free(x->buf);
  }
}

static void *faust_new(t_symbol *s, int argc, t_atom *argv)
{
  t_faust *x = (t_faust*)pd_new(faust_class);
  int sr = -1;
  t_symbol *id = NULL;
  x->active = 1;
  for (int i = 0; i < argc; i++)
    if (argv[i].a_type == A_FLOAT || argv[i].a_type == A_DEFFLOAT)
      sr = (int)argv[i].a_w.w_float;
    else if (argv[i].a_type == A_SYMBOL || argv[i].a_type == A_DEFSYMBOL)
      id = argv[i].a_w.w_symbol;
  x->rate = sr;
  if (sr <= 0) sr = 44100;
  x->xfade = 0; x->n_xfade = (int)(sr*XFADE_TIME/64);
  x->inputs = x->outputs = x->buf = NULL;
  x->label = new string(sym(mydsp) "~");
  x->dsp = new mydsp();
  x->ui = new PdUI(id?id->s_name:NULL);
  if (!x->dsp || !x->ui || !x->label) goto error;
  if (id) {
    *x->label += " ";
    *x->label += id->s_name;
  }
  x->n_in = x->dsp->getNumInputs();
  x->n_out = x->dsp->getNumOutputs();
  if (x->n_in > 0)
    x->inputs = (t_sample**)malloc(x->n_in*sizeof(t_sample*));
  if (x->n_out > 0) {
    x->outputs = (t_sample**)malloc(x->n_out*sizeof(t_sample*));
    x->buf = (t_sample**)malloc(x->n_out*sizeof(t_sample*));
  }
  if ((x->n_in > 0 && x->inputs == NULL) ||
      (x->n_out > 0 && (x->outputs == NULL || x->buf == NULL)))
    goto error;
  for (int i = 0; i < x->n_out; i++)
    x->buf[i] = NULL;
  x->dsp->init(sr);
  x->dsp->buildUserInterface(x->ui);
  for (int i = 0; i < x->n_in; i++)
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  x->out = outlet_new(&x->x_obj, 0);
  for (int i = 0; i < x->n_out; i++)
    outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
 error:
  faust_free(x);
  x->dsp = NULL; x->ui = NULL;
  x->inputs = x->outputs = x->buf = NULL;
  return (void *)x;
}

extern "C" void faust_setup(mydsp)
{
  t_symbol *s = gensym(sym(mydsp) "~");
  faust_class =
    class_new(s, (t_newmethod)faust_new, (t_method)faust_free,
	      sizeof(t_faust), CLASS_DEFAULT,
	      A_GIMME, A_NULL);
  class_addmethod(faust_class, (t_method)faust_dsp, gensym((char*)"dsp"), A_NULL);
  class_addanything(faust_class, faust_any);
  class_addmethod(faust_class, nullfn, &s_signal, A_NULL);
  s_button = gensym((char*)"button");
  s_checkbox = gensym((char*)"checkbox");
  s_vslider = gensym((char*)"vslider");
  s_hslider = gensym((char*)"hslider");
  s_nentry = gensym((char*)"nentry");
  s_vbargraph = gensym((char*)"vbargraph");
  s_hbargraph = gensym((char*)"hbargrap");
  /* give some indication that we're loaded and ready to go */
  mydsp dsp = mydsp();
  post("[faust] %s: %d inputs, %d outputs", sym(mydsp) "~",
       dsp.getNumInputs(), dsp.getNumOutputs());
}
