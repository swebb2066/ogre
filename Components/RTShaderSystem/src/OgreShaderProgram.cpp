/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreShaderPrecompiledHeaders.h"

namespace Ogre {
namespace RTShader {

//-----------------------------------------------------------------------------
Program::Program(GpuProgramType type, const String& desc)
{
    mType               = type;
    // all programs must have an entry point, nobody cares about FFT
    mEntryPointFunction = new Function("main", desc, Function::FFT_VS_MAIN);
    mSkeletalAnimation  = false;
    mColumnMajorMatrices = true;

    mFunctions.push_back(mEntryPointFunction);
}

//-----------------------------------------------------------------------------
Program::~Program()
{
    destroyParameters();

    destroyFunctions();
}

//-----------------------------------------------------------------------------
void Program::destroyParameters()
{
    mParameters.clear();
}

//-----------------------------------------------------------------------------
void Program::destroyFunctions()
{
    ShaderFunctionIterator it;

    for (it = mFunctions.begin(); it != mFunctions.end(); ++it)
    {
        OGRE_DELETE *it;
    }
    mFunctions.clear();
}

//-----------------------------------------------------------------------------
GpuProgramType Program::getType() const
{
    return mType;
}

//-----------------------------------------------------------------------------
void Program::addParameter(UniformParameterPtr parameter)
{
    if (getParameterByName(parameter->getName()).get() != NULL)
    {
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, 
            "Parameter <" + parameter->getName() + "> already declared in program.", 
            "Program::addParameter" );
    }

    mParameters.push_back(parameter);
}

//-----------------------------------------------------------------------------
void Program::removeParameter(UniformParameterPtr parameter)
{
    UniformParameterIterator it;

    for (it = mParameters.begin(); it != mParameters.end(); ++it)
    {
        if ((*it) == parameter)
        {
            (*it).reset();
            mParameters.erase(it);
            break;
        }
    }
}

//-----------------------------------------------------------------------------

static bool isArray(GpuProgramParameters::AutoConstantType autoType)
{
    switch (autoType)
    {
    case GpuProgramParameters::ACT_WORLD_MATRIX_ARRAY_3x4:
    case GpuProgramParameters::ACT_WORLD_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_WORLD_DUALQUATERNION_ARRAY_2x4:
    case GpuProgramParameters::ACT_WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4:
    case GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_SPECULAR_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_ATTENUATION_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POSITION_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POSITION_OBJECT_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POSITION_VIEW_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIRECTION_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIRECTION_OBJECT_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIRECTION_VIEW_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DISTANCE_OBJECT_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POWER_SCALE_ARRAY:
    case GpuProgramParameters::ACT_SPOTLIGHT_PARAMS_ARRAY:
    case GpuProgramParameters::ACT_DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_CASTS_SHADOWS_ARRAY:
    case GpuProgramParameters::ACT_TEXTURE_VIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_SHADOW_SCENE_DEPTH_RANGE_ARRAY:
        return true;
    default:
        return false;
    }
}

UniformParameterPtr Program::resolveParameter(GpuProgramParameters::AutoConstantType autoType, size_t data)
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);

    size_t size = 0;
    if(isArray(autoType)) std::swap(size, data); // for array autotypes the extra parameter is the size

    if (param && param->getAutoConstantIntData() == data)
    {
        return param;
    }
    
    // Create new parameter
    param = UniformParameterPtr(OGRE_NEW UniformParameter(autoType, data, size));
    addParameter(param);

    return param;
}

UniformParameterPtr Program::resolveAutoParameterReal(GpuProgramParameters::AutoConstantType autoType, 
                                                Real data, size_t size)
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);
    if (param.get() != NULL)
    {
        if (param->isAutoConstantRealParameter() &&
            param->getAutoConstantRealData() == data)
        {
            param->setSize(std::max(size, param->getSize()));
            return param;
        }
    }
    
    // Create new parameter.
    param = UniformParameterPtr(OGRE_NEW UniformParameter(autoType, data, size));
    addParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
UniformParameterPtr Program::resolveAutoParameterReal(GpuProgramParameters::AutoConstantType autoType, GpuConstantType type,
                                                Real data, size_t size)
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);
    if (param.get() != NULL)
    {
        if (param->isAutoConstantRealParameter() &&
            param->getAutoConstantRealData() == data)
        {
            param->setSize(std::max(size, param->getSize()));
            return param;
        }
    }
    
    // Create new parameter.
    param = UniformParameterPtr(OGRE_NEW UniformParameter(autoType, data, size, type));
    addParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
UniformParameterPtr Program::resolveAutoParameterInt(GpuProgramParameters::AutoConstantType autoType, GpuConstantType type, 
                                           size_t data, size_t size)
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);
    if (param.get() != NULL)
    {
        if (param->isAutoConstantIntParameter() &&
            param->getAutoConstantIntData() == data)
        {
            param->setSize(std::max(size, param->getSize()));
            return param;
        }
    }

    // Create new parameter.
    param = UniformParameterPtr(OGRE_NEW UniformParameter(autoType, data, size, type));
    addParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
UniformParameterPtr Program::resolveParameter(GpuConstantType type, 
                                    int index, uint16 variability,
                                    const String& suggestedName,
                                    size_t size)
{
    UniformParameterPtr param;

    if (index == -1)
    {
        index = 0;

        // Find the next available index of the target type.
        UniformParameterIterator it;

        for (it = mParameters.begin(); it != mParameters.end(); ++it)
        {
            if ((*it)->getType() == type &&
                (*it)->isAutoConstantParameter() == false)
            {
                index++;
            }
        }
    }
    else
    {
        // Check if parameter already exists.
        param = getParameterByType(type, index);
        if (param.get() != NULL)
        {       
            return param;       
        }
    }
    
    // Create new parameter.
    param = ParameterFactory::createUniform(type, index, variability, suggestedName, size);
    addParameter(param);

    return param;
}



//-----------------------------------------------------------------------------
UniformParameterPtr Program::getParameterByName(const String& name)
{
    UniformParameterIterator it;

    for (it = mParameters.begin(); it != mParameters.end(); ++it)
    {
        if ((*it)->getName() == name)
        {
            return *it;
        }
    }

    return UniformParameterPtr();
}

//-----------------------------------------------------------------------------
UniformParameterPtr Program::getParameterByType(GpuConstantType type, int index)
{
    UniformParameterIterator it;

    for (it = mParameters.begin(); it != mParameters.end(); ++it)
    {
        if ((*it)->getType() == type &&
            (*it)->getIndex() == index)
        {
            return *it;
        }
    }

    return UniformParameterPtr();
}

//-----------------------------------------------------------------------------
UniformParameterPtr Program::getParameterByAutoType(GpuProgramParameters::AutoConstantType autoType)
{
    UniformParameterIterator it;

    for (it = mParameters.begin(); it != mParameters.end(); ++it)
    {
        if ((*it)->isAutoConstantParameter() && (*it)->getAutoConstantType() == autoType)
        {
            return *it;
        }
    }

    return UniformParameterPtr();
}

//-----------------------------------------------------------------------------
Function* Program::createFunction(const String& name, const String& desc, const Function::FunctionType functionType)
{
    Function* shaderFunction;

    shaderFunction = getFunctionByName(name);
    if (shaderFunction != NULL)
    {
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, 
            "Function " + name + " already declared in program.", 
            "Program::createFunction" );
    }

    shaderFunction = OGRE_NEW Function(name, desc, functionType);
    mFunctions.push_back(shaderFunction);

    return shaderFunction;
}

//-----------------------------------------------------------------------------
Function* Program::getFunctionByName(const String& name)
{
    ShaderFunctionIterator it;

    for (it = mFunctions.begin(); it != mFunctions.end(); ++it)
    {
        if ((*it)->getName() == name)
        {
            return *it;
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------
void Program::addDependency(const String& libFileName)
{
    for (unsigned int i=0; i < mDependencies.size(); ++i)
    {
        if (mDependencies[i] == libFileName)
        {
            return;
        }
    }
    mDependencies.push_back(libFileName);
}

void Program::addPreprocessorDefines(const String& defines)
{
    mPreprocessorDefines +=
        mPreprocessorDefines.empty() ? defines : ("," + defines);
}

//-----------------------------------------------------------------------------
size_t Program::getDependencyCount() const
{
    return mDependencies.size();
}

//-----------------------------------------------------------------------------
const String& Program::getDependency(unsigned int index) const
{
    return mDependencies[index];
}

}
}
