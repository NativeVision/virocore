//
//  VROChoreographer.h
//  ViroKit
//
//  Created by Raj Advani on 8/9/17.
//  Copyright © 2017 Viro Media. All rights reserved.
//

#ifndef VROChoreographer_h
#define VROChoreographer_h

#include <memory>
#include <map>
#include <vector>
#include <functional>
#include "optional.hpp"
#include "VROVector4f.h"
#include "VROViewport.h"

class VROScene;
class VRODriver;
class VROLight;
class VROTexture;
class VRORenderPass;
class VRORenderTarget;
class VRORenderContext;
class VROImagePostProcess;
class VROShaderProgram;
class VROToneMappingRenderPass;
class VROGaussianBlurRenderPass;
class VROPostProcessEffectFactory;
class VRORenderMetadata;
class VRORenderToTextureDelegate;
class VROPreprocess;
class VRORendererConfiguration;
enum class VROPostProcessEffect;
enum class VROEyeType;

class VROChoreographer {
public:
    
    VROChoreographer(VRORendererConfiguration config, std::shared_ptr<VRODriver> driver);
    virtual ~VROChoreographer();
    
    virtual void render(VROEyeType eye,
                        std::shared_ptr<VROScene> scene,
                        std::shared_ptr<VROScene> outgoingScene,
                        const std::shared_ptr<VRORenderMetadata> &metadata,
                        VRORenderContext *context,
                        std::shared_ptr<VRODriver> &driver);
    
    void setBaseRenderPass(std::shared_ptr<VRORenderPass> pass) {
        _baseRenderPass = pass;
    }
    
    /*
     Enable or disable HDR rendering. If HDR is disabled, then features like
     Bloom and PBR will not work, and tone-mapping will be disabled. HDR is not
     supported on all devices. If HDR is not supported, this will return false.
     Defaults to true if supported by the device.
     */
    bool setHDREnabled(bool enableHDR);
    bool isHDREnabled() const { return _hdrEnabled; }
    
    /*
     Enable or disable PBR rendering. If PBR is disabled, then objects using
     VROLightingModelPhysicallyBased will degrade to VROLightingModelBlinn.
     
     PBR is not supported on all devices. If PBR is not supported, this will
     return false. This will also return false if HDR is not enabled.
     Defaults to true if supported by the device.
     */
    bool setPBREnabled(bool enablePBR);
    bool isPBREnabled() const { return _hdrEnabled && _pbrEnabled; }
    
    /*
     Enable or disable rendering shadows. If shadows are disabled here, shadow
     casting lights will simply not cast a shadow.
     
     If shadows are not supported, this will return false. Defaults to true if
     supported by the device.
     */
    bool setShadowsEnabled(bool enableShadows);
    
    /*
     Enable or disable rendering bloom. If bloom is not supported, this will
     return false. Defaults to true if supported by the device.
     */
    bool setBloomEnabled(bool enableBloom);
    
    /*
     Enable or disable RTT. When RTT is enabled, the scene is rendered first
     to an offscreen buffer. Then it is flipped and blitted over to the provided
     texture. This enables other systems to process the rendered scene. The RTT
     callback is invoked each time a frame is rendered.
     
     Note the flip is required so that the texture appears right-side-up in the
     RTT texture.

     TODO VIRO-2025: Remove these functions in favor of setting a VRORenderToTextureDelgateiOS.
     */
    void setRenderToTextureEnabled(bool enabled);
    void setRenderTexture(std::shared_ptr<VROTexture> texture);
    void setRenderToTextureCallback(std::function<void()> callback);
    
    /*
     Render targets need to be recreated when the viewport size is changed. They
     also need to be able to set their viewport when bound.
     */
    void setViewport(VROViewport viewport, std::shared_ptr<VRODriver> &driver);
    
    /*
     Retrieve the configurable tone mapping pass.
     */
    std::shared_ptr<VROToneMappingRenderPass> getToneMapping();

    /*
     Retrieves the factory from which to enable/disable post processing effects applied
     in VROChoreographer::renderBasePass().
     */
    std::shared_ptr<VROPostProcessEffectFactory> getPostProcessEffectFactory();

    /*
     Sets a delegate that is invoked each time a frame has been rendered and passes it
     a reference to the final VRORenderTarget containing a texture representing the rendered scene.
     */
    void setRenderToTextureDelegate(std::shared_ptr<VRORenderToTextureDelegate> delegate);

    /*
     Updates the main set of render targets used in the rendering pipeline with the
     following clear color.
     */
    void setClearColor(VROVector4f color, std::shared_ptr<VRODriver> driver);

private:
    
    std::weak_ptr<VRODriver> _driver;
    std::experimental::optional<VROViewport> _viewport;
    VROVector4f _clearColor;

    /*
     True if the GPU supports multiple render targets.
     */
    bool _mrtSupported;
    
    /*
     True if HDR rendering is supported/enabled. When HDR rendering is
     enabled, the scene is rendered to a floating point texture, then a
     tone-mapping algorithm is applied to preserve details in both bright
     and dark regions of the scene.
     */
    bool _hdrSupported, _hdrEnabled;
    
    /*
     True if PBR rendering is supported/enabled. When PBR is enabled,
     PhysicallyBased materials are rendered using the PBR shaders and
     image-based lighting is activated (if available).
     */
    bool _pbrSupported, _pbrEnabled;
    
    /*
     True if Bloom is supported/enabled. When Bloom is enabled, an additional
     color buffer is bound that receives bright colors via a special bloom shader
     modifier. This buffer is blurred and added back to the scene.
     */
    bool _bloomSupported, _bloomEnabled;
    
    /*
     True if for the next frame render targets need to be recreated.
     */
    bool _renderTargetsChanged;
    
#pragma mark - Render Scene
    
    /*
     Pass that renders the 3D scene to a render target.
     */
    std::shared_ptr<VRORenderPass> _baseRenderPass;

    /*
     Simple blitting post process.
     */
    std::shared_ptr<VROImagePostProcess> _blitPostProcess;
    
    /*
     Intermediate render target used for recording video, and other
     post processes.
     */
    std::shared_ptr<VRORenderTarget> _blitTarget;
    
    /*
     Created the required render targets given the current settings (e.g. _hdrEnabled,
     _pbrEnabled, etc.).
     */
    void createRenderTargets();
    
    /*
     Render the 3D scene (and an optional outgoing scene), and perform post-processing.
     */
    void renderScene(std::shared_ptr<VROScene> scene,
                     std::shared_ptr<VROScene> outgoingScene,
                     const std::shared_ptr<VRORenderMetadata> &metadata,
                     VRORenderContext *context, std::shared_ptr<VRODriver> &driver);
    
#pragma mark - Render to Texture
    
    /*
     RTT variables.
     */
    bool _renderToTexture;
    std::shared_ptr<VRORenderTarget> _renderToTextureTarget;
    std::function<void()> _renderToTextureCallback;
    
    /*
     Render the given tone-mapped and gamma-corrected input to the
     video texture and display.
     */
    void renderToTextureAndDisplay(std::shared_ptr<VRORenderTarget> input,
                                   std::shared_ptr<VRODriver> driver);

    /*
     Delegate set by recorders to be notified of 'blitted' render targets containing texture
     representing the rendered scene that is needed for recording / screen capturing.
     */
    std::shared_ptr<VRORenderToTextureDelegate> _renderToTextureDelegate;
    
#pragma mark - Shadows
    
    /*
     True if shadow maps are enabled.
     */
    bool _shadowsEnabled;
    
#pragma mark - HDR
    
    /*
     Floating point target for initially rendering the scene.
     */
    std::shared_ptr<VRORenderTarget> _hdrTarget;
    
    /*
     Tone mapping render pass to render the floating point scene in RGB.
     */
    std::shared_ptr<VROToneMappingRenderPass> _toneMappingPass;
    
#pragma mark - Bloom
    
    /*
     Render targets for ping-ponging the blur operation.
     */
    std::shared_ptr<VRORenderTarget> _blurTargetA;
    std::shared_ptr<VRORenderTarget> _blurTargetB;
    
    /*
     The size of the blur targets relative to the display. Smaller scale leads to
     less accurate but faster blur.
     */
    float _blurScaling;
    
    /*
     Render pass that iteratively performs Gaussian blur on the two blur targets.
     */
    std::shared_ptr<VROGaussianBlurRenderPass> _gaussianBlurPass;
    
    /*
     Additive blending post process for mapping the blur texture back onto the
     main texture.
     */
    std::shared_ptr<VROImagePostProcess> _additiveBlendPostProcess;

#pragma mark - Additional Post-Process Effects

    /*
     Factory that coordinates the creation and application of post processing effects.
     */
    std::shared_ptr<VROPostProcessEffectFactory> _postProcessEffectFactory;

    /*
     Intermediate targets used for post-processing effects.
     */
    std::shared_ptr<VRORenderTarget> _postProcessTargetA;
    std::shared_ptr<VRORenderTarget> _postProcessTargetB;
    
#pragma mark - Preprocessing
    
    std::vector<std::shared_ptr<VROPreprocess>> _preprocesses;
    
};

#endif /* VROChoreographer_h */
