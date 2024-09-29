/*
  Developer(s): EE24B016, EE24B047, EE24B122,  ChatGPT (for comments and guidance)
  Date: 29th September 2024
  Purpose: Analyze Photon Detector Timestamps to Estimate BER and Visibility with Guard Bands
  Description:
    This program reads photon detection timestamps from a CSV file, bins the data into a 32ns time window,
    and identifies a 3ns window where the total count of timestamps is maximized. It divides the 3ns window
    into three 1ns bins (C1, D1, C2) to estimate the bit error rate (BER1) and visibility (V1).
    The program then applies 100ps guard bands between the 1ns bins, discards timestamps within the guard bands,
    and recalculates the counts to estimate a new BER (BER2) and visibility (V2).

  Input(s):
    - A CSV file containing photon detector timestamps in the first column (in picoseconds).
      The second column (if present) is ignored.

  Output(s):
    - Group, BER1, Visibility1, BER2, and Visibility2 printed to the console.
    - (Optional) Output to a file for future analysis.

  Key Operations:
    - Modulo operation to bin timestamps within a 32ns window.
    - Sliding window algorithm to identify the 3ns window with the maximum count.
    - Guard bands applied between consecutive 1ns bins to discard erroneous data.

  Usage:
    - Compile and run the program by providing a CSV file as input:
      Example:
      ./a.out timestamps.csv

    - CSV format:
      timestamp1, value1
      timestamp2, value2
      (Only the first column is used in the analysis)
*/
/*
  Is 100ps Guard Band Adequate?
  When using timestamps_1.csv, executing the find_optimal_guard_bands() function will yield both the "Optimal Guard Band for Minimum BER" and the "Optimal Guard Band for Maximum Visibility" at 110ps and 100ps respectively.
  Therefore, for the provided timestamps_1.csv, the optimal guard band is determined to be 100ps. 
  (Note that the maximum guard band is set to 300ps to prevent the loss of too many valid signals.)


  General answer:
  Based on typical photon detection timing uncertainties, 100ps seems to provide a good balance. It eliminates erroneous data points without discarding too many valid timestamps. However, depending on the noise characteristics of the system, adjusting the guard band width might yield better results.
  Increasing the guard band (e.g., to 200ps) would further reduce BER but might also remove too many valid signals, lowering visibility.
  Decreasing the guard band (e.g., to 50ps) might allow more valid timestamps to be counted but could increase noise, negatively affecting BER.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h> // For fmod

#define WINDOW_SIZE 32000 // 32ns in picoseconds
#define GUARD_BAND 100    // 100ps guard band
#define GROUP "M"
#define MAX_GUARD_BAND 300 // Maximum guard band in ps
#define MIN_GUARD_BAND 100 // Minimum guard band in ps
#define GUARD_BAND_STEP 1 // Step size for guard bands

// Function to read timestamps from CSV, modulo them by 32000ps, and populate histogram
void process_csv_and_create_histogram(const char *filename, int *histogram)
{
  FILE *file = fopen(filename, "r");
  if (!file)
  {
    printf("Error: Could not open file %s\n", filename);
    exit(1);
  }

  // Skip the first row (header)
  char buffer[1024];
  fgets(buffer, sizeof(buffer), file); // Ignoring the first row (header)

  double timestamp;
  double second_column_value; // To read and discard the second column

  // Initialize histogram with 0
  for (int i = 0; i < WINDOW_SIZE; i++)
  {
    histogram[i] = 0;
  }

  // Read timestamps from the first column of the CSV and ignore the second column
  while (fscanf(file, "%lf,%lf", &timestamp, &second_column_value) == 2)
  {
    // Apply modulo-32000 to fit within 32ns window
    int mod_timestamp = (int)(timestamp) % WINDOW_SIZE;

    // Increment the corresponding bin in the histogram
    histogram[mod_timestamp]++;
  }

  // Close the file
  fclose(file);
}

// Function to find max sum window and calculate BER1 and Visibility1
double find_max_sum_window(int histogram[], int size, int window_size, double *BER1, double *V1)
{
  int max_sum = 0;
  int current_sum = 0;
  int start_index = 0;

  if (window_size > size)
  {
    return -1;
  }

  // Calculate the sum of the first window
  for (int i = 0; i < window_size; i++)
  {
    current_sum += histogram[i];
  }

  max_sum = current_sum;

  // Slide the window and update the sum
  for (int i = window_size; i < size; i++)
  {
    current_sum += histogram[i] - histogram[i - window_size];
    if (current_sum > max_sum)
    {
      max_sum = current_sum;
      start_index = i - window_size + 1;
    }
  }

  // Divide the window into 3 equal parts (without guard bands)
  int part_size = window_size / 3;
  int C1 = 0, D1 = 0, C2 = 0;

  for (int i = start_index; i < start_index + window_size; i++)
  {
    if (i < start_index + part_size)
    {
      C1 += histogram[i];
    }
    else if (i < start_index + 2 * part_size)
    {
      D1 += histogram[i];
    }
    else
    {
      C2 += histogram[i];
    }
  }

  // Calculate BER and visibility
  *BER1 = (double)D1 / (C1 + D1 + C2);
  *V1 = (double)(C1 + C2) / D1;

  return start_index;
}

// Function to apply guard bands and calculate BER2 and Visibility2
void apply_guard_bands_and_calculate(int histogram[], int start_index, int window_size, double *BER2, double *V2, int guard_band)
{
  int part_size = window_size / 3;
  int C1 = 0, D1 = 0, C2 = 0;
  int last_bin_index = start_index + window_size - 1;

  // Apply guard bands (ignore the first and last 100ps of each 1ns bin)
  for (int i = start_index; i <= last_bin_index; i++)
  {
    // If within the guard band, skip the timestamp
    int half_guard_band = (int)guard_band / 2;

    // First bin: only remove timestamps from the last half_guard_band ps
    if (i == 0 && (i % 1000) > (1000 - half_guard_band))
    {
      continue; // Skip last half_guard_band ps of the first bin
    }

    // Last bin: only remove timestamps from the first half_guard_band ps
    if (i == last_bin_index && (i % 1000) < half_guard_band)
    {
      continue; // Skip first half_guard_band ps of the last bin
    }

    if ((i % 1000) < half_guard_band || (i % 1000) > (1000 - half_guard_band))
    {
      continue; // Skip timestamps in the guard band
    }

    // Otherwise, accumulate counts
    if (i < start_index + part_size)
    {
      C1 += histogram[i];
    }
    else if (i < start_index + 2 * part_size)
    {
      D1 += histogram[i];
    }
    else
    {
      C2 += histogram[i];
    }
  }

  // Calculate BER and visibility after applying guard bands
  *BER2 = (double)D1 / (C1 + D1 + C2);
  *V2 = (C1 + C2) / D1 ;
}

// Function to loop through different guard bands and find optimal values
void find_optimal_guard_bands(int histogram[], int start_index, int window_size)
{
  double min_BER = 1.0;        // Initialize with max possible BER (1)
  double max_visibility = 0.0; // Initialize with min possible Visibility (0)
  int optimal_BER_guard_band = 0;
  int optimal_visibility_guard_band = 0;

  // Loop through guard band widths
  for (int guard_band = MIN_GUARD_BAND; guard_band <= MAX_GUARD_BAND; guard_band += GUARD_BAND_STEP)
  {
    double BER, visibility;

    // Calculate BER and visibility for the current guard band
    apply_guard_bands_and_calculate(histogram, start_index, window_size, &BER, &visibility, guard_band);

    // Update minimum BER and corresponding guard band
    if (BER < min_BER)
    {
      min_BER = BER;
      optimal_BER_guard_band = guard_band;
    }

    // Update maximum Visibility and corresponding guard band
    if (visibility > max_visibility)
    {
      max_visibility = visibility;
      optimal_visibility_guard_band = guard_band;
    }
  }

  // Print results
  printf("Optimal Guard Band for Minimum BER: %d ps with BER = %.5f\n", optimal_BER_guard_band, min_BER);
  printf("Optimal Guard Band for Maximum Visibility: %d ps with Visibility = %.5f\n", optimal_visibility_guard_band, max_visibility);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  const char *filename = argv[1]; // Input CSV file name (timestamps in picoseconds)
  int histogram[WINDOW_SIZE];

  // Process the CSV file and populate the histogram
  process_csv_and_create_histogram(filename, histogram);

  double BER1, V1, BER2, V2;

  // Find the 3ns window with the maximum sum and calculate BER1 and Visibility1
  int start_index = find_max_sum_window(histogram, WINDOW_SIZE, 3000, &BER1, &V1);

  // Apply guard bands and calculate BER2 and Visibility2
  apply_guard_bands_and_calculate(histogram, start_index, 3000, &BER2, &V2, GUARD_BAND);

  // Output the results
  printf("%s,%lf,%lf,%lf,%lf\n", GROUP, BER1, V1, BER2, V2);

  // Uncomment the below code to find optimal guard band.
  // find_optimal_guard_bands(histogram, start_index, 3000);
  return 0;
}
