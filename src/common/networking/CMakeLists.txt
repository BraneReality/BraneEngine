#src/common/networking

find_package(OpenSSL REQUIRED)
find_package(asio CONFIG REQUIRED)

add_library(networking STATIC 
	"networking.cpp"
	"connection.cpp")
target_link_libraries(networking PUBLIC OpenSSL::SSL asio::asio)

 if(win32)
    target_compile_definitions(networking PUBLIC _WIN32_WINNT_WIN10=0x0A00 _WIN32_WINNT=0x0A00)
endif(win32)
