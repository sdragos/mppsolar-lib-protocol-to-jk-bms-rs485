#include <string>
#include <string.h>
#include <functional>
#include <esp_log.h>

namespace esphome
{

#ifndef H_COMPONENT
#define H_COMPONENT

    template <int...> struct seq {};
    template<int N, int... S> struct gens : gens<N - 1, N - 1, S...> {};  // NOLINT
    template<int... S> struct gens<0, S...> { using type = seq<S...>; };  // NOLINT

    template <typename... X> class CallbackManager;

    /** Helper class to allow having multiple subscribers to a callback.
     *
     * @tparam Ts The arguments for the callbacks, wrapped in void().
     */
    template <typename... Ts>
    class CallbackManager<void(Ts...)>
    {
    public:
        /// Add a callback to the list.
        void add(std::function<void(Ts...)> &&callback) { this->callbacks_.push_back(std::move(callback)); }

        /// Call all callbacks in this manager.
        void call(Ts... args)
        {
            for (auto &cb : this->callbacks_)
                cb(args...);
        }

        /// Call all callbacks in this manager.
        void operator()(Ts... args) { call(args...); }

    protected:
        std::vector<std::function<void(Ts...)>> callbacks_;
    };

    template <typename... Ts>
    class Automation;

    template <typename... Ts>
    class Trigger
    {
    public:
        /// Inform the parent automation that the event has triggered.
        void trigger(Ts... x)
        {
            if (this->automation_parent_ == nullptr)
                return;
            this->automation_parent_->trigger(x...);
        }
        void set_automation_parent(Automation<Ts...> *automation_parent) { this->automation_parent_ = automation_parent; }

        /// Stop any action connected to this trigger.
        void stop_action()
        {
            if (this->automation_parent_ == nullptr)
                return;
            this->automation_parent_->stop();
        }
        /// Returns true if any action connected to this trigger is running.
        bool is_action_running()
        {
            if (this->automation_parent_ == nullptr)
                return false;
            return this->automation_parent_->is_running();
        }

    protected:
        Automation<Ts...> *automation_parent_{nullptr};
    };

    template <typename... Ts>
    class ActionList;

    template <typename... Ts>
    class Action
    {
    public:
        virtual void play_complex(Ts... x)
        {
            this->num_running_++;
            this->play(x...);
            this->play_next_(x...);
        }
        virtual void stop_complex()
        {
            if (num_running_)
            {
                this->stop();
                this->num_running_ = 0;
            }
            this->stop_next_();
        }
        /// Check if this or any of the following actions are currently running.
        virtual bool is_running() { return this->num_running_ > 0 || this->is_running_next_(); }

        /// The total number of actions that are currently running in this plus any of
        /// the following actions in the chain.
        int num_running_total()
        {
            int total = this->num_running_;
            if (this->next_ != nullptr)
                total += this->next_->num_running_total();
            return total;
        }

    protected:
        friend ActionList<Ts...>;

        virtual void play(Ts... x) = 0;
        void play_next_(Ts... x)
        {
            if (this->num_running_ > 0)
            {
                this->num_running_--;
                if (this->next_ != nullptr)
                {
                    this->next_->play_complex(x...);
                }
            }
        }
        template <int... S>
        void play_next_tuple_(const std::tuple<Ts...> &tuple, seq<S...>)
        {
            this->play_next_(std::get<S>(tuple)...);
        }
        void play_next_tuple_(const std::tuple<Ts...> &tuple)
        {
            this->play_next_tuple_(tuple, typename gens<sizeof...(Ts)>::type());
        }

        virtual void stop() {}
        void stop_next_()
        {
            if (this->next_ != nullptr)
            {
                this->next_->stop_complex();
            }
        }

        bool is_running_next_()
        {
            if (this->next_ == nullptr)
                return false;
            return this->next_->is_running();
        }

        Action<Ts...> *next_{nullptr};

        /// The number of instances of this sequence in the list of actions
        /// that is currently being executed.
        int num_running_{0};
    };

    template <typename... Ts>
    class ActionList
    {
    public:
        void add_action(Action<Ts...> *action)
        {
            if (this->actions_end_ == nullptr)
            {
                this->actions_begin_ = action;
            }
            else
            {
                this->actions_end_->next_ = action;
            }
            this->actions_end_ = action;
        }
        void add_actions(const std::vector<Action<Ts...> *> &actions)
        {
            for (auto *action : actions)
            {
                this->add_action(action);
            }
        }
        void play(Ts... x)
        {
            if (this->actions_begin_ != nullptr)
                this->actions_begin_->play_complex(x...);
        }
        void play_tuple(const std::tuple<Ts...> &tuple) { this->play_tuple_(tuple, typename gens<sizeof...(Ts)>::type()); }
        void stop()
        {
            if (this->actions_begin_ != nullptr)
                this->actions_begin_->stop_complex();
        }
        bool empty() const { return this->actions_begin_ == nullptr; }

        /// Check if any action in this action list is currently running.
        bool is_running()
        {
            if (this->actions_begin_ == nullptr)
                return false;
            return this->actions_begin_->is_running();
        }
        /// Return the number of actions in this action list that are currently running.
        int num_running()
        {
            if (this->actions_begin_ == nullptr)
                return false;
            return this->actions_begin_->num_running_total();
        }

    protected:
        template <int... S>
        void play_tuple_(const std::tuple<Ts...> &tuple, seq<S...>) { this->play(std::get<S>(tuple)...); }

        Action<Ts...> *actions_begin_{nullptr};
        Action<Ts...> *actions_end_{nullptr};
    };

    template <typename... Ts>
    class Automation
    {
    public:
        explicit Automation(Trigger<Ts...> *trigger) : trigger_(trigger) { this->trigger_->set_automation_parent(this); }

        Action<Ts...> *add_action(Action<Ts...> *action) { this->actions_.add_action(action); }
        void add_actions(const std::vector<Action<Ts...> *> &actions) { this->actions_.add_actions(actions); }

        void stop() { this->actions_.stop(); }

        void trigger(Ts... x) { this->actions_.play(x...); }

        bool is_running() { return this->actions_.is_running(); }

        /// Return the number of actions in the action part of this automation that are currently running.
        int num_running() { return this->actions_.num_running(); }

    protected:
        Trigger<Ts...> *trigger_;
        ActionList<Ts...> actions_;
    };
    namespace setup_priority
    {

        /// For communication buses like i2c/spi
        extern const float BUS;
    }

    class Component
    {
        // empty class, just to make the compiler happy when copying code that depends on Component but doesn't really need to depend on it.
    public:
        Component() = default;
        virtual void setup() = 0;
        virtual void loop() = 0;
        virtual void dump_config() = 0;
        virtual float get_setup_priority() const = 0;
    };
#endif

}

