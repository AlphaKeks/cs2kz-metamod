#pragma once

#include <string>
#include <unordered_map>

namespace KZ::API
{
	struct BhopStats
	{
		u32 total = 0;
		u32 perfs = 0;
		u32 perfect_perfs = 0;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(BhopStats, total, perfs, perfect_perfs)
	};

	struct CourseSessionData
	{
		f64 playtime = 0.0;
		u16 started_runs = 0;
		u16 finished_runs = 0;
		BhopStats bhop_stats {};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(CourseSessionData, playtime, started_runs, finished_runs, bhop_stats)
	};

	struct CourseSession
	{
		std::optional<CourseSessionData> vanilla {};
		std::optional<CourseSessionData> classic {};

		friend void to_json(json &j, const CourseSession &session)
		{
			if (session.vanilla.has_value())
			{
				j["vanilla"] = session.vanilla.value();
			}

			if (session.classic.has_value())
			{
				j["classic"] = session.classic.value();
			}
		}
	};

	struct GameSession
	{
		/**
		 * Amount of seconds after which we consider a player to be AFK if they
		 * are not spectating and haven't moved.
		 */
		static constexpr f64 AFK_THRESHOLD = 30.0;

		enum class PlayerState
		{
			ACTIVE,
			SPECTATING,
			AFK,
		};

		GameSession() = default;

		/**
		 * Switches the players state and updates timestamps.
		 */
		void SwitchState(PlayerState newState)
		{
			const f64 now = g_pKZUtils->GetServerGlobals()->realtime;
			f64 delta = now - this->lastStateChangedAt;
			this->lastStateChangedAt = now;

			// If we go AFK now, that means we've been AFK for `AFK_THRESHOLD`
			// seconds. We don't want that extra time to count towards
			// `secondsActive` / `secondsSpectating`.
			if (newState == PlayerState::AFK && this->state != PlayerState::AFK)
			{
				delta -= AFK_THRESHOLD;
			}

			switch (this->state)
			{
				case PlayerState::ACTIVE:
				{
					this->secondsActive += delta;
					break;
				}
				case PlayerState::SPECTATING:
				{
					this->secondsSpectating += delta;
					break;
				}
				case PlayerState::AFK:
				{
					this->secondsAfk += delta;
					break;
				}
			}

			this->state = newState;
		}

		/**
		 * Called whenever the player hits a non-perfect bhop.
		 */
		void RegisterBhop()
		{
			this->bhopStats.total++;
		}

		/**
		 * Called whenever the player hits a perf
		 * (whatever "perf" means according to their current mode).
		 */
		void RegisterPerf()
		{
			this->RegisterBhop();
			this->bhopStats.perfs++;
		}

		/**
		 * Called whenever the player hits a tick-perfect bhop.
		 */
		void RegisterPerfectPerf()
		{
			this->RegisterBhop();
			this->RegisterPerf();
			this->bhopStats.perfect_perfs++;
		}

	private:
		PlayerState state = PlayerState::ACTIVE;
		f64 lastStateChangedAt = 0.0;
		f64 lastMovedAt = 0.0;
		f64 secondsActive = 0.0;
		f64 secondsSpectating = 0.0;
		f64 secondsAfk = 0.0;
		BhopStats bhopStats {};
		std::unordered_map<u16, CourseSession> courseSessions {};

		friend void to_json(json &j, const GameSession &session)
		{
			j["seconds_active"] = session.secondsActive;
			j["seconds_spectating"] = session.secondsSpectating;
			j["seconds_afk"] = session.secondsAfk;
			j["bhop_stats"] = session.bhopStats;
			j["course_sessions"] = json::object();

			for (const auto &[courseID, session] : session.courseSessions)
			{
				j["course_sessions"][std::to_string(courseID)] = session;
			}
		}

		friend void from_json(const json &j, GameSession &session)
		{
			assert(false && "this function is never called");
		}
	};
}; // namespace KZ::API
