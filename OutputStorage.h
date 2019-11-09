class OutputStorage {
  private:
    String time = "";
    String button = "";
  public:
    // time
    void setTime(String time) {
      this->time = time;
    }
    String getTime() {
      return this->time;
    }
    // button
    void setButton(String button) {
      this->button = button;
    }
    String getButton() {
      return this->button;
    }
};
