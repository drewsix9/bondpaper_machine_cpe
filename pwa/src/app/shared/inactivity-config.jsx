// Configuration for app-wide settings
export const INACTIVITY_CONFIG = {
  // Default timeout in milliseconds (5 minutes)
  DEFAULT_TIMEOUT: 5 * 60 * 1000,

  // Test timeout in milliseconds (10 seconds)
  TEST_TIMEOUT: 10 * 1000,

  // Set to true to use test timeout, false to use default
  USE_TEST_TIMEOUT: false,

  // Get the current timeout value based on configuration
  get timeout() {
    return this.USE_TEST_TIMEOUT ? this.TEST_TIMEOUT : this.DEFAULT_TIMEOUT;
  },
};
