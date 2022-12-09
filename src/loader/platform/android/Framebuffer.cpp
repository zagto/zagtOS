#include <Framebuffer.hpp>
#include <memory/ArchRegions.hpp>
#include <iostream>
#include <setup/HandOverState.hpp>
#include <DeviceTree.hpp>

static bool framebufferInitialized{false};
static hos_v1::FramebufferInfo info;

hos_v1::FramebufferInfo &InitFramebuffer(void) {
    deviceTree::Tree tree;
    auto reservedMemoryNode = tree.rootNode.findChildNode("reserved-memory");
    if (!reservedMemoryNode) {
        cout << "could not find reserved-memory node" << endl;
        Panic();
    }
    auto continousSplashNode = reservedMemoryNode->findChildNode("cont_splash_region");
    if (!continousSplashNode) {
        cout << "could not find cont_splash_region node" << endl;
        Panic();
    }
    Region region = continousSplashNode->getRegionProperty();
    auto socNode = tree.rootNode.findChildNode("soc");
    if (!socNode) {
        cout << "could not find soc node" << endl;
        Panic();
    }
    auto displayNode = socNode->findChildWithProperty("qcom,dsi-display", "qcom,dsi-display-active");
    if (!displayNode) {
        cout << "could not find active qcom,dsi-display node" << endl;
        Panic();
    }
    auto panelHandleProperty = displayNode->findProperty("qcom,dsi-panel");
    if (!panelHandleProperty) {
        cout << "could not find qcom,dsi-panel property" << endl;
        Panic();
    }
    auto panelNode = tree.rootNode.findNodeByPHandle(panelHandleProperty->getInt<uint32_t>());
    if (!panelNode) {
        cout << "could not find panel node" << endl;
        Panic();
    }
    auto timingsNode = panelNode->findChildNode("qcom,mdss-dsi-display-timings");
    if (!timingsNode) {
        cout << "could not find qcom,mdss-dsi-display-timings node" << endl;
        Panic();
    }
    /* simply take the first timing node. Hopefully all timings have the same resolution */
    auto timingNode = timingsNode->findChildNode();
    if (!timingNode) {
        cout << "could not find qcom,mdss-dsi-display-timings child node" << endl;
        Panic();
    }
    auto widthProperty = timingNode->findProperty("qcom,mdss-dsi-panel-width");
    if (!widthProperty) {
        cout << "could not find qcom,mdss-dsi-panel-width property" << endl;
        Panic();
    }
    auto heightProperty = timingNode->findProperty("qcom,mdss-dsi-panel-height");
    if (!heightProperty) {
        cout << "could not find qcom,mdss-dsi-panel-height property" << endl;
        Panic();
    }
    auto colorOrderProperty = panelNode->findProperty("qcom,mdss-dsi-color-order");
    if (!colorOrderProperty) {
        cout << "could not find qcom,mdss-dsi-color-order property" << endl;
        Panic();
    }

    info = hos_v1::FramebufferInfo{
        .frontBuffer = reinterpret_cast<uint8_t *>(region.start),
        .backBuffer = nullptr, /* inserted by mapFrameBuffer */
        .width = widthProperty->getInt<uint32_t>(),
        .height = heightProperty->getInt<uint32_t>(),
        .bytesPerPixel = 4,
        .bytesPerLine = widthProperty->getInt<uint32_t>() * 4,
        .format = hos_v1::FramebufferFormat::BGR,
        .scaleFactor = 3,
    };

    framebufferInitialized = true;
    return info;
}

hos_v1::FramebufferInfo &GetFramebuffer() {
    assert(framebufferInitialized);
    return info;
}
