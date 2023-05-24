namespace esphome{
    namespace setup_priority {

        /// For communication buses like i2c/spi
        extern const float BUS;
    }

    class Component{
        //empty class, just to make the compiler happy when copying code that depends on Component but doesn't really need to depend on it.
        public:
            Component() = default;
            virtual void setup() = 0;
            virtual void loop() = 0;
            virtual void dump_config() = 0;
            virtual float get_setup_priority() const = 0;
    };
}