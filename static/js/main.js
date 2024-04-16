document.addEventListener("DOMContentLoaded", function () {
  const buttons = document.querySelectorAll(".button");
  buttons.forEach((button, index) => {
    button.addEventListener("click", function () {
      button.classList.toggle("on");
      const state = button.classList.contains("on");
      fetch("/update", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ index: index, state: state }),
      })
        .then((response) => response.json())
        .then((data) => {
          if (!data.success) {
            button.classList.add("unavailable");
          } else {
            button.classList.remove("unavailable");
          }
        });
    });
  });
});
