#ifndef REALM_BENCHMARK_STATS_HPP_
#define REALM_BENCHMARK_STATS_HPP_

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <filesystem>

class HistogramStats
{
    friend std::ostream &operator<<(std::ostream &out, const HistogramStats &s);

public:
    HistogramStats(std::uint64_t bucket_size, std::size_t num_buckets, bool print_stats_on_exit = false)
        : HistogramStats("<anonymous>", bucket_size, num_buckets, print_stats_on_exit)
    {
    }

    HistogramStats(std::string name, std::uint64_t bucket_size, std::size_t num_buckets,
                   bool print_stats_on_exit = false)
        : label(name), jiffy_size(bucket_size), histogram(std::make_unique<std::uint64_t[]>(num_buckets + 1)),
          histogram_size(num_buckets), running_sum(0), print_on_exit(print_stats_on_exit)
    {
        std::fill(&this->histogram[0], &this->histogram[num_buckets + 1], 0);
    }

    ~HistogramStats()
    {
        if (this->print_on_exit)
        {
            auto file = create_output_file();
            if (file)
            {
                file << *this << std::endl;
            }
            std::cout << *this << std::endl;
        }
    }

    void set_label(const std::string &label, bool print_stats_on_exit = true)
    {
        this->label = label;
        this->print_on_exit = print_stats_on_exit;
    }

    void event(std::uint64_t stat)
    {
        std::uint64_t num_jiffies = (stat + (this->jiffy_size >> 1)) / this->jiffy_size;
        if (num_jiffies < this->histogram_size)
        {
            this->histogram[num_jiffies]++;
        }
        else
        {
            this->histogram[this->histogram_size]++;
        }

        // Overflow shouldn't be a concern, if so divide by 1000 here
        running_sum += stat;
    }

private:
    std::string label;
    std::uint64_t jiffy_size;
    std::unique_ptr<std::uint64_t[]> histogram;
    std::size_t histogram_size;
    std::uint64_t running_sum;
    bool print_on_exit;

    std::ofstream create_output_file() const
    {
        std::string filename = label;
        // Replace spaces and special characters with underscores
        std::replace(filename.begin(), filename.end(), ' ', '_');
        std::replace(filename.begin(), filename.end(), ':', '_');
        filename += ".txt";

        // Open file in truncate mode (ios::trunc) to overwrite if exists
        std::ofstream file(filename, std::ios::out | std::ios::trunc);
        if (!file)
        {
            std::cerr << "Failed to create output file: " << filename << std::endl;
        }
        return file;
    }
};

inline std::ostream &operator<<(std::ostream &out, const HistogramStats &s)
{
    auto oldPrecision = out.precision(2);
    auto oldFlags = out.setf(std::ios::fixed, std::ios::floatfield);

    std::uint64_t number_of_samples = 0;
    for (std::size_t i = 0; i != s.histogram_size + 1; i++)
    {
        number_of_samples += s.histogram[i];
    }

    float mean = s.running_sum / (float)number_of_samples;

    std::uint64_t min_idx = 0;
    std::uint64_t p1_idx = number_of_samples / 100;
    std::uint64_t p10_idx = number_of_samples / 10;
    std::uint64_t p25_idx = number_of_samples / 4;
    std::uint64_t p50_idx = number_of_samples / 2;
    std::uint64_t p75_idx = 3 * number_of_samples / 4;
    std::uint64_t p90_idx = 9 * number_of_samples / 10;
    std::uint64_t p99_idx = 99 * number_of_samples / 100;
    std::uint64_t max_idx = number_of_samples - 1;

    std::uint64_t cutoffs[] = {min_idx, p1_idx, p10_idx, p25_idx, p50_idx, p75_idx, p90_idx, p99_idx, max_idx};
    std::size_t num_cutoffs = 9;
    std::size_t values[9];
    std::fill(&values[0], &values[num_cutoffs], 0);

    std::uint64_t cumulative_count = 0;
    std::size_t current_cutoff = 0;
    for (std::size_t i = 0; i != s.histogram_size && current_cutoff < num_cutoffs; i++)
    {
        cumulative_count += s.histogram[i];
        while (current_cutoff < num_cutoffs && cutoffs[current_cutoff] < cumulative_count)
        {
            values[current_cutoff] = static_cast<std::uint64_t>(i) * s.jiffy_size;
            current_cutoff++;
        }
    }

    out << "Latency = " << mean << " \n"
        << s.label << ": ( count = " << number_of_samples << ", min = " << values[0] << ", p1 = " << values[1]
        << ", p10 = " << values[2] << ", p25 = " << values[3] << ", p50 = " << values[4] << ", p75 = " << values[5]
        << ", p90 = " << values[6] << ", p99 = " << values[7] << ", max = " << values[8]
        << ", high = " << s.histogram[s.histogram_size] << " )";

    // Restore original stream settings
    out.precision(oldPrecision);
    out.flags(oldFlags);

    return out;
}

#endif