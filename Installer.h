#pragma once
#include <memory>
#include <coreinit/mutex.h>
#include <atomic>

using std::shared_ptr;

#define MCP_INSTALL_CANCELLED 0xDEAD0002

typedef union {
    MCPInstallInfo info;
    MCPInstallTitleInfo titleInfo;
    struct {
        void (*mcpCallback)(MCPError err, void *rawData);
        void *installer;
    };
} MCPInstallTitleInfoUnion;

class Installer {
public:
    Installer(Installer const&) = delete;
    Installer& operator=(Installer const&) = delete;
    ~Installer();

    static std::shared_ptr<Installer> instance()
    {
        static std::shared_ptr<Installer> s{new Installer};
        return s;
    }

    std::tuple<int, std::string> install(MCPInstallTarget target, const std::string &wupPath, void (*installProgressCallback)(MCPInstallProgress*, OSTime));
    void cancel();

    MCPError lastErr;
    std::atomic<bool> processing;
private:
    int updateTime = 1000000;
    Installer();
    int mcpHandle;
    bool initialized;
    MCPInstallTitleInfoUnion *info;
    MCPInstallProgress *installProgress;
    OSMutex *installLock;
};