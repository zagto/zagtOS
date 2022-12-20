#include <Framebuffer.hpp>
#include <memory/ArchRegions.hpp>
#include <iostream>
#include <setup/HandOverState.hpp>
#include <DeviceTree.hpp>

static hos_v1::FramebufferInfo info;

bool findQualcommFramenbuffer(const deviceTree::Tree &tree) {
    auto reservedMemoryNode = tree.rootNode.findChildNode("reserved-memory");
    if (!reservedMemoryNode) {
        cout << "No Qualcomm framebuffer: could not find reserved-memory node" << endl;
        return false;
    }
    auto continousSplashNode = reservedMemoryNode->findChildNode("cont_splash_region");
    if (!continousSplashNode) {
        cout << "No Qualcomm framebuffer: could not find cont_splash_region node" << endl;
        return false;
    }
    Region region = continousSplashNode->getRegionProperty();
    auto socNode = tree.rootNode.findChildNode("soc");
    if (!socNode) {
        cout << "No Qualcomm framebuffer: could not find soc node" << endl;
        return false;
    }
    auto displayNode = socNode->findChildWithProperty("qcom,dsi-display", "qcom,dsi-display-active");
    if (!displayNode) {
        cout << "No Qualcomm framebuffer: could not find active qcom,dsi-display node" << endl;
        return false;
    }
    auto panelHandleProperty = displayNode->findProperty("qcom,dsi-panel");
    if (!panelHandleProperty) {
        cout << "No Qualcomm framebuffer: could not find qcom,dsi-panel property" << endl;
        return false;
    }
    auto panelNode = tree.rootNode.findNodeByPHandle(panelHandleProperty->getInt<uint32_t>());
    if (!panelNode) {
        cout << "No Qualcomm framebuffer: could not find panel node" << endl;
        return false;
    }
    auto timingsNode = panelNode->findChildNode("qcom,mdss-dsi-display-timings");
    if (!timingsNode) {
        cout << "No Qualcomm framebuffer: could not find qcom,mdss-dsi-display-timings node" << endl;
        return false;
    }
    /* simply take the first timing node. Hopefully all timings have the same resolution */
    auto timingNode = timingsNode->findChildNode();
    if (!timingNode) {
        cout << "No Qualcomm framebuffer: could not find qcom,mdss-dsi-display-timings child node"
             << endl;
        return false;
    }
    auto widthProperty = timingNode->findProperty("qcom,mdss-dsi-panel-width");
    if (!widthProperty) {
        cout << "No Qualcomm framebuffer: could not find qcom,mdss-dsi-panel-width property"
             << endl;
        return false;
    }
    auto heightProperty = timingNode->findProperty("qcom,mdss-dsi-panel-height");
    if (!heightProperty) {
        cout << "No Qualcomm framebuffer: could not find qcom,mdss-dsi-panel-height property"
             << endl;
        return false;
    }
    auto colorOrderProperty = panelNode->findProperty("qcom,mdss-dsi-color-order");
    if (!colorOrderProperty) {
        cout << "No Qualcomm framebuffer: could not find qcom,mdss-dsi-color-order property"
             << endl;
        return false;
    }

    info = hos_v1::FramebufferInfo{
        .type = hos_v1::FramebufferType::SIMPLE,
        .frontBuffer = reinterpret_cast<uint8_t *>(region.start),
        .backBuffer = nullptr, /* inserted by mapFrameBuffer */
        .width = widthProperty->getInt<uint32_t>(),
        .height = heightProperty->getInt<uint32_t>(),
        .bytesPerPixel = 4,
        .bytesPerLine = widthProperty->getInt<uint32_t>() * 4,
        .format = hos_v1::FramebufferFormat::BGR,
        .scaleFactor = 3,
    };
    return true;
}

hos_v1::FramebufferInfo &InitFramebuffer(void) {
    deviceTree::Tree tree;
    if (findQualcommFramenbuffer(tree)) {
        cout << "Detected framebuffer type: Qualcomm" << endl;
    } else {
        cout << "No framebuffer found" << endl;
        info.type = hos_v1::NO_FRAMEBUFFER;
    }
    return info;
}

hos_v1::FramebufferInfo &GetFramebuffer() {
    return info;
}
