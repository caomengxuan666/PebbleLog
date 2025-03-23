#pragma once
#include <memory>
#include <vector>

namespace utils::Log::MiddleWare {
    class MiddlewareBase {
    public:
        virtual ~MiddlewareBase() = default;
        virtual void sink() = 0;
    };

    template<class MiddlewareType>
    class LoggerMiddleware : public MiddlewareBase {
    public:
        void sink() override {
            static_cast<MiddlewareType*>(this)->process();
        }
    };

    class MiddlewareChain {
        std::vector<std::shared_ptr<MiddlewareBase>> middlewares;
    public:
    template<typename MiddlewareType, typename... Args>
    void addMiddleware(Args&&... args) {
        middlewares.emplace_back(
            std::make_shared<MiddlewareType>(std::forward<Args>(args)...)
        );
    }

        void process() {
            for (const auto& middleware : middlewares) {
                middleware->sink();
            }
        }
    };
}