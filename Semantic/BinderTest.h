/******************************************************************************
 * Copyright (c) 2014-2016 Leandro T. C. Melo (ltcmelo@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 *****************************************************************************/

/*--------------------------*/
/*--- The UaiSo! Project ---*/
/*--------------------------*/

#ifndef UAISO_BINDERTEST_H__
#define UAISO_BINDERTEST_H__

#include "Semantic/Binder.h"
#include "Parsing/Lexeme.h"
#include "Parsing/LexemeMap.h"
#include <string>

namespace uaiso {

class Factory;
class Program;

class Binder::BinderTest final : public Test
{
public:
    TEST_RUN(BinderTest
             // Go
             , &BinderTest::GoTestCase1
             // Python
             , &BinderTest::PyTestCase1
             , &BinderTest::PyTestCase2
             , &BinderTest::PyTestCase3
             , &BinderTest::PyTestCase4
             , &BinderTest::PyTestCase5
             , &BinderTest::PyTestCase6
             , &BinderTest::PyTestCase7
             , &BinderTest::PyTestCase8
             , &BinderTest::PyTestCase9
             , &BinderTest::PyTestCase10
             , &BinderTest::PyTestCase11
             , &BinderTest::PyTestCase12
             )

    //--- Go ---//

    void GoTestCase1();

    //--- Python ---//

    void PyVerifyBasicCode(const Program* prog);
    void PyTestCase1();
    void PyTestCase2();
    void PyTestCase3();
    void PyTestCase4();
    void PyTestCase5();
    void PyTestCase6();
    void PyTestCase7();
    void PyTestCase8();
    void PyTestCase9();
    void PyTestCase10();
    void PyTestCase11();
    void PyTestCase12();


    std::unique_ptr<Program> core(std::unique_ptr<Factory> factory,
                                  const std::string& code,
                                  const std::string& fullFileName);

    void reset() override
    {
        lexs_.clear();
    }

    LexemeMap lexs_;
};

} // namespace uaiso

#endif

