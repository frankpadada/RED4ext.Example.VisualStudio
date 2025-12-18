@wrapMethod(PlayerPuppet)
protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
  let handled = false;

  if action.IsAction(n"FreeLookToggle") && action.IsJustPressed() {
    LogChannel(n"DEBUG", "AimSplit: FreeLookToggle pressed (Backspace)");
    AimSplit_OnAction();
    handled = true;
  }

  return wrappedMethod(action, consumer) || handled;
}
