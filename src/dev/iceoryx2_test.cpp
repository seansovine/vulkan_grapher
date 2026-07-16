#include <iox2/node.hpp>
#include <iox2/service.hpp>
#include <iox2/service_type.hpp>

int main() {
    auto node = iox2::NodeBuilder().create<iox2::ServiceType::Ipc>().value();

    // TODO: Implement dynamic memory request-response pattern.
}
