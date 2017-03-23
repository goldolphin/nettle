set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.6)
project(Nettle)

if(APPLE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -g -O0 -std=gnu99 -Wall -Werror -Wno-error=unused")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -g -O0 -std=gnu++11 -Wall -Werror")
elseif(CYGWIN)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -g -O0 -lrt -std=gnu99 -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -g -O0 -std=gnu++11 -Wall -Werror")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -g -O0 -rdynamic -lrt -std=gnu99 -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -g -O0 -rdynamic -std=gnu++11 -Wall -Werror")
endif()

include_directories(src/main ../third-party/thrift-092/lib/cpp/src E:/boost_1_57_0)

add_library(utils STATIC src/main/utils/logger.h src/main/utils/logger.c src/main/utils/bitmap.h src/main/utils/bitmap.c src/main/utils/heap.h src/main/utils/heap.c
    src/main/utils/metric.h src/main/utils/metric.c src/main/utils/sbuilder.h src/main/utils/sbuilder.c src/main/utils/memory.h
    src/main/utils/queue.c src/main/utils/queue.h src/main/utils/conqueue.c src/main/utils/conqueue.h src/main/utils/hashmap.h src/main/utils/hashmap.c
    src/main/utils/list.h src/main/utils/list.c src/main/utils/lrumap.h src/main/utils/lrumap.c
    src/main/utils/config.h src/main/utils/config.c src/main/utils/utils.h src/main/utils/pair_t.h src/main/utils/pair_t.c)

add_library(rs STATIC src/main/rs/galois.h src/main/rs/galois.c src/main/rs/rscode.h src/main/rs/rscode.c src/main/rs/rs.h src/main/rs/rs.c
)
target_link_libraries(rs utils)

add_library(dispatcher STATIC src/main/dispatcher/common.h src/main/dispatcher/common.c src/main/dispatcher/udp.c src/main/dispatcher/udp.h
    src/main/dispatcher/dispatcher.h src/main/dispatcher/dispatcher.c src/main/dispatcher/worker.h src/main/dispatcher/worker.c
    src/main/dispatcher/pool.h src/main/dispatcher/pool.c src/main/dispatcher/counter.h src/main/dispatcher/counter.c src/main/dispatcher/perf.h src/main/dispatcher/perf.c
        src/main/dispatcher/tun_tap.c src/main/dispatcher/tun_tap.h)
target_link_libraries(dispatcher utils)

add_library(tunnel STATIC src/main/tunnel/nt_fec.h src/main/tunnel/nt_fec.c src/main/tunnel/nt_rs_fec.h src/main/tunnel/nt_rs_fec.c
        src/main/tunnel/nt_dup_fec.h src/main/tunnel/nt_dup_fec.c src/main/tunnel/nt_distributor.h src/main/tunnel/nt_distributor.c
        src/main/tunnel/nt_error.h src/main/tunnel/nt_perf.h src/main/tunnel/nt_perf.c src/main/tunnel/nt_fec_processor.h src/main/tunnel/nt_fec_processor.c
        src/main/tunnel/nt_proto.h src/main/tunnel/nt_proto.c src/main/tunnel/nt_listener.h src/main/tunnel/nt_listener.c
        src/main/tunnel/nt_reporter.h src/main/tunnel/nt_reporter.c src/main/tunnel/nt_forwarder.h src/main/tunnel/nt_forwarder.c
        src/main/tunnel/nt_client.h src/main/tunnel/nt_client.c src/main/tunnel/nt_monitor.h src/main/tunnel/nt_monitor.c src/main/tunnel/nt_echo_listener.h src/main/tunnel/nt_echo_listener.c src/main/tunnel/nt_proto2.c src/main/tunnel/nt_proto2.h)
target_link_libraries(tunnel rs dispatcher)

add_library(proxy STATIC src/main/proxy/nt-proxy.c src/main/proxy/nt-proxy.h)
target_link_libraries(proxy tunnel)

add_executable(nt-tunnel src/main/tunnel/main.c)
target_link_libraries(nt-tunnel tunnel)

add_executable(nt-proxy src/main/proxy/main.c)
target_link_libraries(nt-proxy proxy)

add_executable(nt-client src/main/tools/nt_client_main.c)
target_link_libraries(nt-client tunnel)

add_library(echo src/main/echo/packet.h src/main/tools/nt_echo_main.c src/main/echo/udp_server.c src/main/echo/udp_client.c src/main/echo/udp_cs.h)
target_link_libraries(echo tunnel)

add_executable(nt-echo src/main/tools/nt_echo_main.c)
target_link_libraries(nt-echo echo)

add_executable(nt-monitor src/main/tools/nt_monitor_main.c)
target_link_libraries(nt-monitor echo)

add_executable(nt-config src/main/tools/nt_config_main.c)
target_link_libraries(nt-config utils)

add_executable(nt-tuntap src/main/tools/nt_tun_tap_main.c)
target_link_libraries(nt-tuntap dispatcher)

enable_testing()

add_executable(test-galois src/test/test_galois.c)
target_link_libraries(test-galois rs)
add_test(test-galois test-galois)

add_executable(test-rscode src/test/test_rscode.c)
target_link_libraries(test-rscode rs)
add_test(test-rscode test-rscode)

add_executable(test-rs src/test/test_rs.c)
target_link_libraries(test-rs rs)
add_test(test-rs test-rs)

add_executable(perf-rscode src/test/perf_rscode.c)
target_link_libraries(perf-rscode rs)
add_test(perf-rscode perf-rscode)

add_executable(perf-rs src/test/perf_rs.c)
target_link_libraries(perf-rs rs)
add_test(perf-rs perf-rs)

add_executable(test-lrumap  src/test/test_lrumap.c)
target_link_libraries(test-lrumap utils)
add_test(test-lrumap test-lrumap)

add_executable(test-config  src/test/test_config.c)
target_link_libraries(test-config utils)
add_test(test-config test-config)

configure_file(src/test/conf/config.test src/test/conf/config.test)
configure_file(bin/start-proxy.sh start-proxy.sh)
