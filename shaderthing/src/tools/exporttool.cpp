#include "shaderthingapp.h"
#include "layers/layer.h"
#include "tools/exporttool.h"
#include "data/data.h"

#include "thirdparty/stb/stb_image_write.h"
//#include "thirdparty/opencv/include/opencv2/opencv.hpp"

#include <iomanip>

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Static members ------------------------------------------------------------//

std::unordered_map<ExportTool::ExportType, std::string> 
    ExportTool::exportTypeToName = {
        //{ExportTool::ExportType::Video, "Video"},
        {ExportTool::ExportType::VideoFrames, "Video frames"},
        {ExportTool::ExportType::GIF, "GIF"},
        {ExportTool::ExportType::Image, "Image"}
    };

std::unordered_map<int, std::string> 
    ExportTool::gifDitheringLevelToName = {
        {2, "Ordered (Bayer) 4x4"},
        {1, "Ordered (Bayer) 2x2"},
        {0, "None"}
    };

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

// Constructor/destructor ----------------------------------------------------//

ExportTool::ExportTool
(
    ShaderThingApp& app
) :
    app_(app),
    isGuiOpen_(false),
    isGuiInMenu_(true),
    isExporting_(false),
    exportType_(ExportType::Image),
    time_(app.timeRef()),
    timePaused_(app.isTimePausedRef()),
    exportStartTime_(0),
    exportEndTime_(1.0),
    exportFps_(25.0),
    frame_(0),
    nPreExportFrames_(0),
    updateLayers_(false),
    layers_(app.layersRef()),
    exportFileExtension_(""),
    exportFilepathNoExtension_(""),
    exportFramebuffer_(nullptr),
    exportFramebufferData_(nullptr),
    exportLayerData_(0),
    gifPaletteBitDepth_(8),
    gifDitheringLevel_(0),
    nRendersPerFrame_(1),
    multipleRendersOnlyOnFirstFrame_(true)
{}

ExportTool::~ExportTool()
{
    if (exportFramebuffer_ != nullptr)
        delete exportFramebuffer_;
    exportFramebuffer_ = nullptr;
    if (exportFramebufferData_ != nullptr)
        delete[] exportFramebufferData_;
    exportFramebufferData_ = nullptr;
}

//----------------------------------------------------------------------------//

void ExportTool::update()
{
    // Update outputLayerResolutions, do it the stupid (but self-contained) way
    // for now
    if (!isExporting_)
    {
        if (!isGuiOpen_)
            return;
        for (auto layer : layers_)
        {
            if (exportLayerData_.find(layer) == exportLayerData_.end())
            {
                auto eld = ExportLayerData{};
                eld.resolution = glm::ivec2(layer->resolution());
                eld.resolutionScale = layer->resolutionScale();
                eld.resolutionLocked = false;
                exportLayerData_.insert({layer, eld});
            }
            else if (updateLayers_)
            {
                ExportLayerData& eld = exportLayerData_[layer];
                if (eld.resolutionLocked)
                    continue;
                eld.resolution = layer->resolution();
                eld.resolutionScale = layer->resolutionScale();
            }
        }
        if (updateLayers_)
            updateLayers_ = false;
        std::vector<Layer*> toBeRemoved(0);
        for (auto& e : exportLayerData_)
        {
            auto layer = e.first;
            if 
            (
                std::find(layers_.begin(), layers_.end(), layer) == 
                layers_.end()
            )
                toBeRemoved.push_back(layer);
        }
        for (auto& layer : toBeRemoved)
            exportLayerData_.erase(layer);
        return;
    }
    // else if (isExporting)
    if (frame_ == 0) // Setup
    {
        // Disable VSync to avoid loosing time when rendering to the export 
        // framebuffer
        vir::GlobalPtr<vir::Window>::instance()->setVSync(false);
        if (exportFramebuffer_ != nullptr)
            delete exportFramebuffer_;
        if (exportFramebufferData_ != nullptr)
            delete[] exportFramebufferData_; // Just in case
        exportFramebuffer_ = vir::Framebuffer::create
        (
            exportResolution_.x,
            exportResolution_.y
        );
        exportFramebufferData_ = new unsigned char
        [
            exportFramebuffer_->colorBufferDataSize()
        ];
        
        nExportFrames_ = 0;//nPreExportFrames_;
        if (exportType_ != ExportType::Image)
            nExportFrames_ += (exportEndTime_-exportStartTime_)*exportFps_+.5f;
        else
            nExportFrames_ += 1;
        if (exportType_ != ExportType::Image)
        {
            time_ = exportStartTime_ - 
                ((double)(2))/(double)exportFps_; //nPreExportFrames_+2
            timePaused_ = false;
        }
        
        // Resize layers
        for (auto& e : exportLayerData_)
        {
            Layer* layer = e.first;
            ExportLayerData& data = e.second;
            if (data.resolutionLocked)
                continue;
            
            data.backupResolution = layer->resolution();
            layer->setTargetResolution(data.resolution, false);
            layer->update(); // If this is not done here, the first frame will
                             // be rendered at the original layer resolution
        }
    }
    // The +1 is to account for the fact that, since self-rendered layers are
    // double buffered, I need to populate the read-only framebuffer as well. 
    // Thus, I force one extra frame of rendering just in case
    else if (frame_ > 1) // nPreExportFrames_+1
    {
        exportFrame();
        if (frame_ >= nExportFrames_+1) // nExportFrames_+1
        {
            isExporting_ = false;
            frame_ = 0;
            delete exportFramebuffer_;
            exportFramebuffer_ = nullptr;
            delete[] exportFramebufferData_;
            exportFramebufferData_ = nullptr;
            vir::GlobalPtr<vir::Window>::instance()->setVSync(true);
            for (auto& e : exportLayerData_)
            {
                Layer* layer = e.first;
                ExportLayerData& data = e.second;
                if (data.resolutionLocked)
                    continue;
                layer->setTargetResolution(data.backupResolution, false);
                layer->update();
            }
            return;
        }
    }
    frame_++;
}

//----------------------------------------------------------------------------//

void ExportTool::exportFrame()
{
    if 
    (
        !isExporting_ || 
        exportFilepathNoExtension_ == "" ||
        exportFramebuffer_ == nullptr || 
        exportFramebufferData_ == nullptr
    )
        return;
    
    // Retrieve data
    if (exportType_ != ExportType::GIF)
        exportFramebuffer_->colorBufferData(exportFramebufferData_, true);

    // Save it to disk depending on the format
    std::string exportFilename
    (
        exportFilepathNoExtension_+
        exportFileExtension_
    );
    switch (exportType_)
    {
        case (ExportType::Image) :
        case (ExportType::VideoFrames) :
        {
            if (exportType_ == ExportType::VideoFrames)
            {
                // Save frame FPS and frame number in out path as well. If the 
                // FPS is not an integer, the '.' is replaced by a 'd' (for 'dot')
                // so not to mess with the file extension
                std::stringstream fpsStream;
                fpsStream << std::fixed << std::setprecision(2) << exportFps_;
                std::string fps = fpsStream.str()+"fps";
                std::replace(fps.begin(), fps.end(), '.', 'd');
                exportFilename = 
                    exportFilepathNoExtension_ + "_" + fps + "_" +
                    std::to_string(frame_-2) + // frame_-nPreExportFrames_-2
                    exportFileExtension_;
            }
            stbi_write_png
            (
                exportFilename.c_str(), 
                exportResolution_.x,
                exportResolution_.y,
                4, 
                (const void*)exportFramebufferData_, 
                exportResolution_.x*4
            );
            return;
        }
        case (ExportType::GIF) :
        {
            static bool setup(true);
	        int delay = std::max(100.0f/exportFps_, 1.0f);
            static vir::GifEncoder* gifEncoder(nullptr);
            if (setup)
            {
                gifEncoder = new vir::GifEncoder();
                gifEncoder->openFile
                ( 
                    exportFilename.c_str(), 
                    exportResolution_.x, 
                    exportResolution_.y, 
                    gifPaletteBitDepth_
                );
                setup = false;
            }
            gifEncoder->encodeFrame
            (
                exportFramebuffer_,
                delay,
                gifDitheringLevel_,
                0.0f,
                true
            );
            if (frame_ >= nExportFrames_+1)
            {
                gifEncoder->closeFile();
                delete gifEncoder;
                gifEncoder = nullptr;
                setup = true;
            }
            return;
        }
        // This OpenCV magic to export videos directly works, but due to
        // licensing issues (at least as far as I've been able to understand),
        // I can only export in some sub-par-ish-somewhat-over-compressed .mp4
        // format. This, coupled with the desire to keep third-party bloat as 
        // little as possible, and considering that including the entirety of
        // OpenCV just to get the aforemetioned 'eh' video-export capability,
        // made me abandon the idea. You can export the video frames (see 
        // ExportType::VideoFrames) individually and glue them up in the 
        // software of your choice, I find that cleaner
        /*case (ExportType::Video) :
        {
            static cv::VideoWriter* videoWriter(nullptr);
            static bool setupWriter(true);
            if (setupWriter)
            {
                videoWriter = new cv::VideoWriter
                (
                    exportFilename,
                    cv::VideoWriter::fourcc('M','J','P','G'),
                    exportFps_,
                    cv::Size(exportResolution_.x, exportResolution_.y)
                );
                setupWriter = false;
            }
            cv::Mat frame
            (
                exportResolution_.y, 
                exportResolution_.x, 
                CV_8UC4, 
                exportFramebufferData_
            );
            videoWriter->write(frame);
            if (frame_ >= nExportFrames_+1)
            {
                videoWriter->release();
                delete videoWriter;
                videoWriter = nullptr;
                setupWriter = true;
            }
            return;
        }*/
    }
}

//----------------------------------------------------------------------------//

void ExportTool::reset()
{
    exportLayerData_.clear();
    updateLayers_ = true;
    setExportResolution(app_.resolutionRef());
}

//----------------------------------------------------------------------------//

void ExportTool::loadState(std::string& source, uint32_t& index)
{
    reset();
}

//----------------------------------------------------------------------------//

void ExportTool::saveState(std::ofstream& file)
{
}

//----------------------------------------------------------------------------//

int ExportTool::nRendersPerFrame()
{
    // Technically the 'first frame' is the first two, because of 
    // double-buffering
    if (multipleRendersOnlyOnFirstFrame_ && frame_ > 1)
        return 1;
    return nRendersPerFrame_;
}

//----------------------------------------------------------------------------//

// One-way update from the App to the Exporter to be called on window size
// changes
void ExportTool::setExportResolution(glm::ivec2 resolution)
{
    exportResolution_ = resolution;
    updateLayers_ = true;
}

//----------------------------------------------------------------------------//

void ExportTool::updateLayerResolutions()
{
    updateLayers_ = true;
}


}