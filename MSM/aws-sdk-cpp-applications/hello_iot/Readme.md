cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=~/aws-sdk-cpp-install \
  -G "Unix Makefiles"



make

export AWS_ACCESS_KEY_ID=AKIARWU63RB6REHZZTVA AWS_SECRET_ACCESS_KEY=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

./hello_iot



===============================
cmake ~/aws-sdk-cpp \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=~/aws-sdk-cpp-install \
  -DBUILD_ONLY="iot" \
  -DENABLE_UNITY_BUILD=ON \
  -DENABLE_TESTING=OFF

cmake --build .
cmake --install .