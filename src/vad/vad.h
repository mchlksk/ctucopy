/*
 CtuCopy 3 - Universal feature extractor and speech enhancer.
 Copyright 2012 Petr Fousek, FEE CTU Prague

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#ifndef VAD_H_
#define VAD_H_

#include <math.h>
#include <linux/limits.h>

#include "../io/opts.h"
#include "../base/types.h"

//-------------------------
#define DELETE(ptr) \
    if(ptr) { \
        delete ptr; \
        ptr = NULL; \
    }
//-------------------------

//------------------------------------------------------------------------------

class FileWriter {
  private:
    char buff[PATH_MAX];
    FILE *fp;

  public:
    FileWriter(const char* filename, const char* suffix = NULL)
    {
        const char* ptr;

        if(suffix != NULL)
        {
            snprintf(buff, sizeof(buff), "%s_%s", filename, suffix);
            ptr = &buff[0];
        }
        else
            ptr = filename;

        if(!(fp = fopen(ptr, "wb")))
            throw("FileWriter: cannot open file!");
    }

    ~FileWriter()
    {
        fclose(fp);
    }

    void write(bool B) {
        char CH = (B) ? '1' : '0';
        fwrite((void*)&CH, sizeof(CH), 1, fp);
    }

    void write(double D) {
        fwrite((void*)&D, sizeof(D), 1, fp);
    }
};

//------------------------------------------------------------------------------

class medianFilter {

private:
    const int order;
    bool *history;
    unsigned int historyIdx;
    unsigned int historySize;

public:
    medianFilter(int ORDER) :
      order(ORDER)
    {
        if(order < 1 || (order % 2) == 0)
            throw("medianFilter: filter order must be positive, odd number!");

        history = new bool[order];
        historyIdx = 0;
        historySize = 0;
    }

    ~medianFilter()
    {
        delete[] history;
    }

    bool push(bool value)
    {
        history[historyIdx] = value;
        historyIdx = (historyIdx+1) % order;
        if (historySize < order)
            historySize++;
        double sum = 0.0;
        for(int i=0; i<historySize; i++)
            sum += (history[i]) ? 1.0 : 0.0;

        return ((sum/double(historySize)) >= 0.5);
    }
};

//------------------------------------------------------------------------------

class VADcri {
    // --- INTERNAL CLASS, please use the one below! -------
    // PROTOTYPE BASE CLASS for all VAD criterial classes

  protected:
        Vec<double>* const xsabsvec;
        Vec<double>* const xsphvec;
        Vec<double>* const infvec;
        Vec<double>* const feafvec;

  public:
        VADcri(const opts* options, Vec<double> *Xsabs, Vec<double> *Xsph, Vec<double> *InFvec, Vec<double> *FeaFvec)
        : xsabsvec(Xsabs),
          xsphvec(Xsph),
          infvec(InFvec),
          feafvec(FeaFvec)
        {};

        virtual ~VADcri() {};

        virtual void new_file(const char*) {return;};  // announces new file
        virtual double process_frame(int) {return 0.0;};
        virtual void save_frame(int) {return;};           // write debug output
        virtual void consume_vad(int, bool) {return;};
};

//------------------------------------------------------------------------------

class VADthr {
    // --- INTERNAL CLASS, please use the one below! -------
    // PROTOTYPE BASE CLASS for all VAD threshold classes
public:
        VADthr(const opts* options) {};
        virtual ~VADthr() {};

        virtual void new_file(const char*) {return;};  // announces new file
        virtual bool process_frame(int, double) {return false;};
        virtual void save_frame(int) {return;};           // write debug output
};

//------------------------------------------------------------------------------

class VAD {
  // this is the only class intended to be public
  // it implements all needed methods using subclasses <x>VAD

private:
        const char *opt_apply_mode;
        const char *opt_out_mode;
        const int opt_filter_order;
        Vec<double> * const xsabsvec; // for silence_frame()

        VADcri* vadcri;
        VADthr* vadthr;
        medianFilter *vadfilter;

        bool vad0; // unfiltered vad
        bool vad;

        FileWriter *file_vad0;
        FileWriter *file_vad;

        int frame_index;

public:
        VAD(const opts*, Vec<double>*, Vec<double>*, Vec<double>*, Vec<double>*);
        ~VAD();

        void new_file(const char*);                       // announces new file
        bool process_frame();                             // detect voice activity in one frame
        void save_frame();                                // save VAD result in output file
        bool drop_frame();
        void silence_frame();                             // silence non-speech frame
};

#endif /*VAD_H_*/

