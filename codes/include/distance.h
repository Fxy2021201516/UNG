#ifndef DISTANCE
#define DISTANCE

#include <memory>
#include <immintrin.h>
#include <x86intrin.h>
#include "config.h"



namespace ANNS {

    // virtual class for distance functions
    class DistanceHandler {
        public:
            virtual float compute(const char *a, const char *b, IdxType dim) const = 0;
            virtual ~DistanceHandler() {}
    };

    
    // get desired distance handler
    std::unique_ptr<DistanceHandler> get_distance_handler(const std::string& data_type, const std::string& dist_fn);
    

    // float L2 distance
    class FloatL2DistanceHandler : public DistanceHandler {
        public:
            float compute(const char *a, const char *b, IdxType dim) const;
        private:
            static inline __m128 masked_read(IdxType dim, const float *x);
    };
}

#endif // DISTANCE