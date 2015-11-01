/******************************************************************************
 * Copyright (c) 2014-2015 Leandro T. C. Melo (ltcmelo@gmail.com)
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

#include "Parsing/Factory.h"
#include "Common/Assert.h"
#include "D/DFactory.h"
#include "Go/GoFactory.h"
#include "Python/PyFactory.h"

using namespace uaiso;

Factory::~Factory()
{}

std::unique_ptr<Factory> FactoryCreator::create(LangName langName)
{
    switch (langName) {
    case LangName::D:
        return std::unique_ptr<Factory>(new DFactory);
    case LangName::Go:
        return std::unique_ptr<Factory>(new GoFactory);
    case LangName::Py:
        return std::unique_ptr<Factory>(new PyFactory);
    default:
        UAISO_ASSERT(false, {});
    }
}
