/*
 * Copyright (c) 2019, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 *
 */

//!
//! \file:   OmafTracksSelector.cpp
//! \brief:  Tracks selector base class implementation
//!
//! Created on May 28, 2019, 1:19 PM
//!

#include "OmafTracksSelector.h"

VCD_OMAF_BEGIN

OmafTracksSelector::OmafTracksSelector(int size)
{
    pthread_mutex_init(&mMutex, NULL);
    mSize = size;
    m360ViewPortHandle = nullptr;
    mParamViewport = nullptr;
    mPose = nullptr;
    mUsePrediction = false;
    mPredictPluginName = "";
    mLibPath = "";
}

OmafTracksSelector::~OmafTracksSelector()
{
    pthread_mutex_destroy( &mMutex );

    if(m360ViewPortHandle)
    {
        I360SCVP_unInit(m360ViewPortHandle);
        m360ViewPortHandle = nullptr;
    }

    SAFE_DELETE(mParamViewport);

    if(mPoseHistory.size())
    {
        for(auto &p:mPoseHistory)
        {
            SAFE_DELETE(p.pose);
        }

        mPoseHistory.clear();
    }

    if (mPredictPluginMap.size())
    {
        for (auto &p:mPredictPluginMap)
        {
            SAFE_DELETE(p.second);
        }

        mPredictPluginMap.clear();
    }

    mUsePrediction = false;

    SAFE_DELETE(mPose);
}

int OmafTracksSelector::SetInitialViewport(
    std::vector<Viewport*>& pView,
    HeadSetInfo* headSetInfo,
    OmafMediaStream* pStream)
{
    if(!headSetInfo || !headSetInfo->viewPort_hFOV || !headSetInfo->viewPort_vFOV
        || !headSetInfo->viewPort_Width || !headSetInfo->viewPort_Height)
    {
        return ERROR_INVALID;
    }

    mParamViewport = new param_360SCVP;
    mParamViewport->usedType = E_VIEWPORT_ONLY;
    mParamViewport->paramViewPort.viewportWidth = headSetInfo->viewPort_Width;
    mParamViewport->paramViewPort.viewportHeight = headSetInfo->viewPort_Height;
    mParamViewport->paramViewPort.viewPortPitch = headSetInfo->pose->pitch;
    mParamViewport->paramViewPort.viewPortYaw = headSetInfo->pose->yaw;
    mParamViewport->paramViewPort.viewPortFOVH = headSetInfo->viewPort_hFOV;
    mParamViewport->paramViewPort.viewPortFOVV = headSetInfo->viewPort_vFOV;
    mParamViewport->paramViewPort.geoTypeInput = (EGeometryType)headSetInfo->input_geoType;
    mParamViewport->paramViewPort.geoTypeOutput = (EGeometryType)headSetInfo->output_geoType;
    mParamViewport->paramViewPort.tileNumRow = pStream->GetRowSize();
    mParamViewport->paramViewPort.tileNumCol = pStream->GetColSize();
    mParamViewport->paramViewPort.usageType = E_VIEWPORT_ONLY;
    mParamViewport->paramViewPort.faceWidth = pStream->GetStreamHighResWidth();
    mParamViewport->paramViewPort.faceHeight = pStream->GetStreamHighResHeight();
    m360ViewPortHandle = I360SCVP_Init(mParamViewport);
    if(!m360ViewPortHandle)
        return ERROR_NULL_PTR;

    //set current Pose;
    mPose = new HeadPose;
    if (!mPose)
        return ERROR_NULL_PTR;

    memcpy(mPose, headSetInfo->pose, sizeof(HeadPose));

    return UpdateViewport(mPose);
}

int OmafTracksSelector::UpdateViewport(HeadPose *pose)
{
    if (!pose)
        return ERROR_NULL_PTR;

    pthread_mutex_lock(&mMutex);

    PoseInfo pi;
    pi.pose = new HeadPose;
    if (!(pi.pose))
        return ERROR_NULL_PTR;
    memcpy(pi.pose, pose, sizeof(HeadPose));
    std::chrono::high_resolution_clock clock;
    pi.time = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
    mPoseHistory.push_front(pi);
    if( mPoseHistory.size() > (uint32_t)(this->mSize) )
    {
        auto pit = mPoseHistory.back();
        SAFE_DELETE(pit.pose);
        mPoseHistory.pop_back();
    }

    pthread_mutex_unlock(&mMutex);
    return ERROR_NONE;
}

int OmafTracksSelector::EnablePosePrediction(
    std::string predictPluginName,
    std::string libPath)
{
    mUsePrediction = true;
    mPredictPluginName.assign(predictPluginName);
    mLibPath.assign(libPath);
    int ret = InitializePredictPlugins();
    return ret;
}

int OmafTracksSelector::InitializePredictPlugins()
{
    if (mLibPath.empty() || mPredictPluginName.empty())
    {
        LOG(ERROR)<<"Viewport predict plugin path OR name is invalid!"<<endl;
        return ERROR_INVALID;
    }
    ViewportPredictPlugin *plugin = new ViewportPredictPlugin();
    if (!plugin)
        return ERROR_NULL_PTR;

    std::string pluginPath = mLibPath + mPredictPluginName;
    int ret = plugin->LoadPlugin(pluginPath.c_str());
    if (ret != ERROR_NONE)
    {
        LOG(ERROR)<<"Load plugin failed!"<<endl;
        SAFE_DELETE(plugin);
        return ret;
    }
    mPredictPluginMap.insert(std::pair<std::string, ViewportPredictPlugin*>(mPredictPluginName, plugin));
    return ERROR_NONE;
}

VCD_OMAF_END